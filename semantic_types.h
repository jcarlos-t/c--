#ifndef SEMANTIC_TYPES_H
#define SEMANTIC_TYPES_H

#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

// Jerarquía de tipos semánticos (Type → PointerType, ArrayType, StructType)
// TypeChecker convierte TypeNode del AST a Type*.

class Type {
public:
    enum TType { NOTYPE, VOID, INT, CHAR, BOOL, FLOAT, DOUBLE, LONG, POINTER, ARRAY, STRUCT };
    TType ttype;       // discriminante de subclase
    bool isUnsigned = false;
    bool isConst = false;

    Type(TType tt) : ttype(tt) {}
    virtual ~Type() = default;

    virtual bool match(Type* t) const {
        return ttype == t->ttype && isUnsigned == t->isUnsigned;
    }
    virtual string str() const {
        string prefix = isConst ? "const " : "";
        string suffix = isUnsigned ? "unsigned " : "";
        string name;
        switch (ttype) {
            case VOID:   name = "void"; break;
            case INT:    name = "int"; break;
            case CHAR:   name = "char"; break;
            case BOOL:   name = "bool"; break;
            case FLOAT:  name = "float"; break;
            case DOUBLE: name = "double"; break;
            case LONG:   name = "long long"; break;
            default:     name = "unknown"; break;
        }
        return prefix + suffix + name;
    }
    virtual int size() const {
        switch (ttype) {
            case VOID:   return 0;
            case INT:    return 4;
            case CHAR:   return 1;
            case BOOL:   return 1;
            case FLOAT:  return 4;
            case DOUBLE: return 8;
            case LONG:   return 8;
            case POINTER: return 8;
            default:     return 8;
        }
    }
};

class PointerType : public Type {
public:
    Type* base;  // tipo apuntado
    PointerType(Type* b) : Type(POINTER), base(b) {}

    bool match(Type* t) const override {
        if (t->ttype != POINTER) return false;
        return base->match(((PointerType*)t)->base);
    }
    string str() const override {
        return base->str() + "*";
    }
};

class ArrayType : public Type {
public:
    Type* base;
    int length;  // ≥0 fijo, -1 desconocido
    ArrayType(Type* b, int l = -1) : Type(ARRAY), base(b), length(l) {}

    bool match(Type* t) const override {
        if (t->ttype != ARRAY) return false;
        return base->match(((ArrayType*)t)->base);
    }
    string str() const override {
        if (length >= 0) return base->str() + "[" + to_string(length) + "]";
        return base->str() + "[]";
    }
    int size() const override {
        if (length >= 0) return base->size() * length;
        return 8;
    }
};

class StructType : public Type {
public:
    string name;
    unordered_map<string, Type*> members;  // nombre del campo → tipo
    StructType(const string& n) : Type(STRUCT), name(n) {}

    bool match(Type* t) const override {
        if (t->ttype != STRUCT) return false;
        return name == ((StructType*)t)->name;
    }
    string str() const override {
        return "struct " + name;
    }
    int size() const override {
        int total = 0;
        for (auto& pair : members)
            total += pair.second->size();
        return total;
    }
};

#endif // SEMANTIC_TYPES_H
