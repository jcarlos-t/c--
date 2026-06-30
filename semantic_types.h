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
    enum TType { NOTYPE, VOID, INT, CHAR, BOOL, FLOAT, DOUBLE, POINTER, ARRAY, STRUCT };
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
            case CHAR:   return "char";
            case BOOL:   return "bool";
            case FLOAT:  return "float";
            case DOUBLE: return "double";
            default:     return "unknown";
        }
    }

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
    int length; // -1 si tamaño desconocido
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

    int size() const override {
        int total = 0;
        for (auto& pair : members) {
            total += pair.second->size();
        }
        return total;
    }
};

#endif // SEMANTIC_TYPES_H
