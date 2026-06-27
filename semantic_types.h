#ifndef SEMANTIC_TYPES_H
#define SEMANTIC_TYPES_H

#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

// ===========================================================
//  Jerarquía de tipos semánticos del lenguaje
//  Soporta: primitivos, punteros, arreglos, structs
// ===========================================================

class Type {
public:
    enum TType { NOTYPE, VOID, INT, BOOL, FLOAT, POINTER, ARRAY, STRUCT };
    TType ttype;

    Type(TType tt) : ttype(tt) {}
    virtual ~Type() = default;

    // Comparación estructural de tipos (virtual para subtipos)
    virtual bool match(Type* t) const {
        return this->ttype == t->ttype;
    }

    virtual string str() const {
        switch (ttype) {
            case VOID:   return "void";
            case INT:    return "int";
            case BOOL:   return "bool";
            case FLOAT:   return "float";
            default:     return "unknown";
        }
    }
};

// Tipo puntero: base es el tipo apuntado
class PointerType : public Type {
public:
    Type* base;
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

// Tipo arreglo: base + tamaño (opcional)
class ArrayType : public Type {
public:
    Type* base;
    int size; // -1 si tamaño desconocido
    ArrayType(Type* b, int s = -1) : Type(ARRAY), base(b), size(s) {}
    ~ArrayType() override { }

    bool match(Type* t) const override {
        if (t->ttype != ARRAY) return false;
        ArrayType* at = (ArrayType*)t;
        return base->match(at->base);
    }

    string str() const override {
        if (size >= 0)
            return base->str() + "[" + to_string(size) + "]";
        return base->str() + "[]";
    }
};

// Tipo struct: contiene nombre y mapa de miembros
class StructType : public Type {
public:
    string name;
    unordered_map<string, Type*> members;

    StructType(const string& n) : Type(STRUCT), name(n) {}
    ~StructType() override { }

    bool match(Type* t) const override {
        if (t->ttype != STRUCT) return false;
        StructType* st = (StructType*)t;
        return name == st->name; // equivalencia por nombre
    }

    string str() const override {
        return "struct " + name;
    }
};

#endif // SEMANTIC_TYPES_H
