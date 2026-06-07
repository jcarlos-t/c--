#ifndef AST_H
#define AST_H

#include <string>
#include <unordered_map>
#include <list>
#include <ostream>
#include <vector>
#include "semantic_types.h"

using namespace std;

class Visitor;
class TypeVisitor;
class Body;

// Operadores binarios soportados
enum BinaryOp { 
    PLUS_OP, 
    MINUS_OP, 
    MUL_OP, 
    DIV_OP,
    LE_OP,
    AND_OP,
    POW_OP
};

// ============================================================
// Clase abstracta Exp
// ============================================================
class Exp {
public:
    virtual double accept(Visitor* visitor) = 0;
    virtual ~Exp() = 0;
    static string binopToChar(BinaryOp op);

    virtual Type* accept(TypeVisitor* visitor) = 0;
};

// ============================================================
// Expresión binaria
// ============================================================
class BinaryExp : public Exp {
public:
    Exp* left;
    Exp* right;
    BinaryOp op;

    BinaryExp(Exp* l, Exp* r, BinaryOp op);
    ~BinaryExp();

    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

// ============================================================
// Expresión numérica
// ============================================================
class NumberExp : public Exp {
public:
    double value;
    bool is_float;

    NumberExp(double v, bool f = false);
    ~NumberExp();

    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

// ============================================================
// Expresión Booleana
// ============================================================
class BoolExp : public Exp {
public:
    int value;

    BoolExp(){};
    ~BoolExp(){};

    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};


// ============================================================
// Expresión de identificador
// ============================================================
class IdExp : public Exp {
public:
    string value;

    IdExp(string v);
    ~IdExp();

    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

// ============================================================
// Llamada a función
// ============================================================
class FcallExp : public Exp {
public:
    string nombre;
    list<Exp*> argumentos;

    FcallExp(){};
    ~FcallExp(){};

    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

// ============================================================
// Clase base para sentencias
// ============================================================
class Stm {
public:
    virtual int accept(Visitor* visitor) = 0;
    virtual ~Stm() = 0;

    // --- NUEVO ---
    virtual void accept(TypeVisitor* visitor) = 0;
};

// ============================================================
// Sentencias
// ============================================================
class AssignStm : public Stm {
public:
    string id;
    Exp* e;

    AssignStm(string, Exp*);
    ~AssignStm();

    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor); // nuevo
};

class PrintStm : public Stm {
public:
    Exp* e;

    PrintStm(Exp*);
    ~PrintStm();

    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor); // nuevo
};

class ReturnStm : public Stm {
public:
    Exp* e;

    ReturnStm() {};
    ~ReturnStm() {};

    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor); // nuevo
};

class WhileStm : public Stm {
public:
    Exp* condition;
    Body* body;

    WhileStm(Exp* c, Body* b);
    ~WhileStm();

    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

class IfStm : public Stm {
public:
    Exp* condition;
    Body* thenBody;
    Body* elseBody;

    IfStm(Exp* c, Body* t, Body* e);
    ~IfStm();

    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

// ============================================================
// Declaraciones, cuerpos y programa
// ============================================================
class VarDec {
public:
    string tipo;
    list<string> variables;

    VarDec() {};
    ~VarDec() {};
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor); // nuevo
};

class Body {
public:
    list<Stm*> stmlist;
    list<VarDec*> vdlist;

    Body() {};
    ~Body() {};
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor); // nuevo
};

class FunDec {
public:
    string tipo;
    string nombre;
    vector<string> Tparametros;
    vector<string> Nparametros;
    Body* cuerpo;

    FunDec() {};
    ~FunDec() {};
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor); // nuevo
};

class Program {
public:
    list<FunDec*> fdlist;
    list<VarDec*> vdlist;

    Program();
    ~Program();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor); // nuevo
};

#endif // AST_H
