#ifndef SEMANTIC_TYPES_H
#define SEMANTIC_TYPES_H

#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

// ===========================================================
//  Jerarquía de tipos semánticos del lenguaje C--
//
//  Representa tipos ya resueltos (post-análisis sintáctico).
//  Cada nodo del AST que representa una expresión o
//  declaración tiene un `resolvedType` apuntando a uno
//  de estos objetos.
//
//  Soporta:
//    - Tipos primitivos: void, int, char, bool, float, double
//    - Punteros:         int*, char*, struct Persona*, etc.
//    - Arreglos:         int[10], char[5][3], etc.
//    - Structs:          struct Persona { ... }
//
//  Ejemplo de uso:
//    Type* t = new ArrayType(new IntType, 10);
//    t->str() => "int[10]"
//    t->size() => 40  (4 bytes * 10)
// ===========================================================

// -----------------------------------------------------------
// Type (abstracta base) — un tipo semántico del lenguaje
// -----------------------------------------------------------
// Provee la interfaz común: comparación estructural (match),
// representación textual (str), y tamaño en bytes (size).
// La clase `TType` enumera todas las variantes concretas.
class Type {
public:
    // Enumeración de todos los tipos soportados
    enum TType { NOTYPE, VOID, INT, CHAR, BOOL, FLOAT, DOUBLE, POINTER, ARRAY, STRUCT };
    TType ttype;  // discriminante del tipo concreto

    Type(TType tt) : ttype(tt) {}
    virtual ~Type() = default;

    // match: comparación estructural de tipos
    //   Ej: int match int  => true
    //       int match char => false
    //       int* match int* => true (PointerType::match recursivo)
    //       int[5] match int[3] => true (ArrayType::match ignora tamaño)
    //       struct A match struct A => true (StructType::match por nombre)
    virtual bool match(Type* t) const {
        return this->ttype == t->ttype;
    }

    // str: representación textual legible
    //   int, char, float, double, bool, void
    //   int*, int[10], struct Persona, etc.
    virtual string str() const {
        switch (ttype) {
            case VOID:   return "void";
            case INT:    return "int";
            case CHAR:   return "char";
            case BOOL:   return "bool";
            case FLOAT:  return "float";
            case DOUBLE: return "double";
            default:     return "unknown";
        }
    }

    // size: tamaño en bytes del tipo
    //   int, float → 4
    //   char, bool → 1
    //   double, puntero, struct → 8 (struct varía)
    //   arreglo    → base->size() * length
    virtual int size() const {
        switch (ttype) {
            case VOID:   return 0;
            case INT:    return 4;
            case CHAR:   return 1;
            case BOOL:   return 1;
            case FLOAT:  return 4;
            case DOUBLE: return 8;
            case POINTER: return 8;
            default:     return 8;
        }
    }
};

// -----------------------------------------------------------
// PointerType — puntero a otro tipo
// -----------------------------------------------------------
// Ejemplo:
//   int x; &x → PointerType(IntType)
//   char* p; → PointerType(CharType)
//   struct A* q; → PointerType(StructType("A"))
//
// match: dos punteros son iguales si apuntan al mismo tipo base
// str:   "int*", "char*", "struct A*"
// size:  siempre 8 bytes (x86-64)
class PointerType : public Type {
public:
    Type* base;  // tipo al que apunta
    PointerType(Type* b) : Type(POINTER), base(b) {}
    ~PointerType() override { }

    bool match(Type* t) const override {
        if (t->ttype != POINTER) return false;
        PointerType* pt = (PointerType*)t;
        return base->match(pt->base);
    }

    string str() const override {
        return base->str() + "*";
    }
};

// -----------------------------------------------------------
// ArrayType — arreglo de tamaño fijo o desconocido
// -----------------------------------------------------------
// Ejemplos:
//   int a[10];        → ArrayType(IntType, 10)
//   int m[2][3];      → ArrayType(ArrayType(IntType, 3), 2)
//   int arr[];        → ArrayType(IntType, -1)   // tamaño desconocido
//   char s[5][3][10]; → ArrayType(ArrayType(ArrayType(CharType, 10), 3), 5)
//
// match: se compara solo el tipo base, ignora el tamaño
//   int[5] match int[3] → true
// str:   "int[10]", "int[3][2]", "char[]"
// size:  base->size() * length (si length >= 0), 8 si es dinámico
class ArrayType : public Type {
public:
    Type* base;    // tipo de cada elemento
    int length;    // número de elementos (-1 si tamaño desconocido)
    ArrayType(Type* b, int l = -1) : Type(ARRAY), base(b), length(l) {}
    ~ArrayType() override { }

    bool match(Type* t) const override {
        if (t->ttype != ARRAY) return false;
        ArrayType* at = (ArrayType*)t;
        return base->match(at->base);
    }

    string str() const override {
        if (length >= 0)
            return base->str() + "[" + to_string(length) + "]";
        return base->str() + "[]";
    }

    int size() const override {
        if (length >= 0) return base->size() * length;
        return 8; // array sin tamaño conocido, tratar como puntero
    }
};

// -----------------------------------------------------------
// StructType — tipo struct con nombre y miembros
// -----------------------------------------------------------
// Ejemplo:
//   struct Persona { char nombre[50]; int edad; };
//   → StructType("Persona")
//     members["nombre"] = ArrayType(CharType, 50)
//     members["edad"]   = IntType
//
// match: igualdad por nombre (equivalencia nominal)
// str:   "struct Persona"
// size:  suma de tamaños de todos los miembros
class StructType : public Type {
public:
    string name;                                    // nombre del struct
    unordered_map<string, Type*> members;           // nombre del miembro → tipo

    StructType(const string& n) : Type(STRUCT), name(n) {}
    ~StructType() override { }

    // Dos structs son iguales si tienen el mismo nombre (equivalencia nominal)
    bool match(Type* t) const override {
        if (t->ttype != STRUCT) return false;
        StructType* st = (StructType*)t;
        return name == st->name;
    }

    string str() const override {
        return "struct " + name;
    }

    // size: suma acumulada de los tamaños de todos los miembros
    int size() const override {
        int total = 0;
        for (auto& pair : members) {
            total += pair.second->size();
        }
        return total;
    }
};

#endif // SEMANTIC_TYPES_H
