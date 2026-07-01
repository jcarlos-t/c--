#ifndef SEMANTIC_TYPES_H
#define SEMANTIC_TYPES_H

#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

// ===========================================================
// Jerarquía de tipos semánticos del lenguaje C--
// ===========================================================
// Representa tipos **ya resueltos** tras el análisis semántico.
// No confundir con los nodos TypeNode del AST (ast.h), que describen
// tipos en la sintaxis fuente (int, char*, struct X, Box<int>).
//
// TypeChecker convierte TypeNode → Type* (type_from_ast) y guarda el
// resultado en Exp::resolvedType / VarDecl::resolvedType.
// GenCodeVisitor lee resolvedType y memSize para emitir movb/movl/movq.
//
// Jerarquía de clases:
//   Type          — base (primitivos + discriminante ttype)
//   PointerType   — T*
//   ArrayType     — T[N] o T[] (anidado para multidimensional)
//   StructType    — struct Nombre { miembros }
//
// Ciclo de vida:
//   - Primitivos: singletons en TypeChecker (intType, charType, ...)
//   - Compuestos: new en typeCache; delete en ~TypeChecker
//   - StructType en struct_types: también liberados en destructor
//
// Métodos comunes:
//   match(Type*) — igualdad de tipos (reglas distintas por subclase)
//   str()        — representación legible para mensajes de error
//   size()       — tamaño en bytes (layout de stack / structs)

// -----------------------------------------------------------
// Type — clase base de tipos semánticos
// -----------------------------------------------------------
class Type {
public:
    // NOTYPE: valor interno por defecto / error; no es un tipo del lenguaje.
    enum TType { NOTYPE, VOID, INT, CHAR, BOOL, FLOAT, DOUBLE, POINTER, ARRAY, STRUCT };
    TType ttype;  // discriminante: indica qué subclase lógica es

    Type(TType tt) : ttype(tt) {}
    virtual ~Type() = default;

    // match en la clase base: solo compara el discriminante ttype.
    // Para punteros, arreglos y structs las subclases sobreescriben
    // con comparación estructural o nominal.
    //
    //   int.match(int)   → true
    //   int.match(char)  → false  (aunque char se promueva en expresiones)
    //   int*.match(int*) → true si bases coinciden (PointerType)
    virtual bool match(Type* t) const {
        return this->ttype == t->ttype;
    }

    // Nombre legible del tipo primitivo. Subclases añaden *, [], struct ...
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

    // Tamaño en bytes para layout (convención del compilador C--):
    //   int/float = 4, char/bool = 1, double/puntero = 8
    // Subclases (ArrayType, StructType) calculan tamaños compuestos.
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
// PointerType — puntero a otro tipo (T*)
// -----------------------------------------------------------
// Ejemplos:
//   int* p     → PointerType(intType)
//   char** pp  → PointerType(PointerType(charType))
//   void* (malloc) → PointerType(voidType)
//
// match: recursivo en el tipo base (int* solo coincide con int*).
// size: 8 bytes en x86-64 (no sobreescribe; usa case POINTER en Type).
class PointerType : public Type {
public:
    Type* base;  // tipo apuntado (no es propietario; lo gestiona TypeChecker)
    PointerType(Type* b) : Type(POINTER), base(b) {}
    ~PointerType() override { }  // no delete base (compartido / en typeCache)

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
// ArrayType — arreglo de elementos (T[N] o T[])
// -----------------------------------------------------------
// Multidimensional se modela anidando ArrayType:
//   int m[2][3] → ArrayType(ArrayType(int, 3), 2)
//
// length:
//   >= 0  tamaño fijo conocido (int a[10])
//   -1    tamaño desconocido o flexible (int a[])
//
// match: compara solo el tipo base; **ignora length**
//   int[5].match(int[3]) → true (misma regla que C para asignación laxa)
//
// size: base->size() * length; si length < 0 → 8 (tratado como puntero)
class ArrayType : public Type {
public:
    Type* base;
    int length;  // -1 si desconocido

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
        return 8;  // arreglo sin tamaño conocido → como puntero en stack
    }
};

// -----------------------------------------------------------
// StructType — tipo struct con nombre y tabla de miembros
// -----------------------------------------------------------
// Equivalencia **nominal**: dos structs son iguales si tienen el mismo
// nombre, no si tienen la misma estructura de campos.
//
//   struct Point en un TU vs otro con mismos campos pero distinto nombre
//   → no match (a diferencia de equivalencia estructural).
//
// members se llena en TypeChecker::visit(StructDecl) o instantiate_template.
// StructDecl también guarda memberOffsets/memberSizes para GenCode.
//
// Instancias de templates usan nombres mangled: "Box<int>", "Par<float>".
class StructType : public Type {
public:
    string name;
    unordered_map<string, Type*> members;  // nombre del campo → Type*

    StructType(const string& n) : Type(STRUCT), name(n) {}
    ~StructType() override { }

    bool match(Type* t) const override {
        if (t->ttype != STRUCT) return false;
        StructType* st = (StructType*)t;
        return name == st->name;
    }

    string str() const override {
        return "struct " + name;
    }

    // Suma lineal de tamaños de miembros (sin padding explícito entre campos).
    int size() const override {
        int total = 0;
        for (auto& pair : members) {
            total += pair.second->size();
        }
        return total;
    }
};

#endif // SEMANTIC_TYPES_H
