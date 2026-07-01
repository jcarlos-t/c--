#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include "semantic_types.h"

using namespace std;

class Visitor;
class VarDecl;
class TypeNode;

struct Location {
    int line;
    int column;
    Location(int l = 0, int c = 0) : line(l), column(c) {}
};

// Operadores binarios, unarios y de asignación
enum class BinaryOp {
    ADD, SUB, MUL, DIV, MOD,
    EQ, NE, LT, GT, LE, GE,
    LOG_AND, LOG_OR, POW
};

enum class UnaryOp {
    ADDR, DEREF, MINUS, LOG_NOT, PRE_INC, PRE_DEC, POST_INC, POST_DEC
};

enum class AssignOp {
    ASSIGN
};

// Clase base para todos los nodos de expresión
class Exp {
public:
    Location loc;
    bool isConstant = false;
    double constantValue = 0.0;
    Type* resolvedType = nullptr;  // tipo semántico resuelto por TypeChecker
    
    Exp() {}
    virtual ~Exp() {}
    virtual void accept(Visitor* visitor);
    virtual void computeAddress(Visitor* visitor);
};

// --- Nodos de expresión ---

// Operación binaria (a + b, a == b, etc.)
class BinaryOpNode : public Exp {
public:
    BinaryOp op;
    Exp* left;
    Exp* right;
    BinaryOpNode(Exp* l, Exp* r, BinaryOp op);
    ~BinaryOpNode();
    void accept(Visitor* visitor);

};

// Operación unaria (*p, -x, !b, ++i, etc.)
class UnaryOpNode : public Exp {
public:
    UnaryOp op;
    Exp* operand;
    UnaryOpNode(Exp* o, UnaryOp op);
    ~UnaryOpNode();
    void accept(Visitor* visitor);

    void computeAddress(Visitor* visitor);
};

// Asignación: =, +=, -=, *=, /=
class AssignmentNode : public Exp {
public:
    AssignOp op;
    Exp* target;
    Exp* value;
    AssignmentNode(Exp* t, Exp* v, AssignOp op = AssignOp::ASSIGN);
    ~AssignmentNode();
    void accept(Visitor* visitor);

};



// Llamada a función: callee(args...)
class FcallNode : public Exp {
public:
    Exp* callee; // nombre
    vector<Exp*> args; // argumentos que recibe la llamada
    FcallNode(Exp* c);
    ~FcallNode();
    void accept(Visitor* visitor);

};

// Acceso por índice: base[index]
class IndexNode : public Exp {
public:
    Exp* base;
    Exp* index;
    IndexNode(Exp* b, Exp* i);
    ~IndexNode();
    void accept(Visitor* visitor);

    void computeAddress(Visitor* visitor);
};

// Acceso a miembro de struct: obj.member
class MemberAccessNode : public Exp {
public:
    Exp* object;
    string member;
    MemberAccessNode(Exp* o, const string& m);
    ~MemberAccessNode();
    void accept(Visitor* visitor);

    void computeAddress(Visitor* visitor);
};

// Acceso a miembro por puntero: ptr->member
class ArrowAccessNode : public Exp {
public:
    Exp* pointer;
    string member;
    ArrowAccessNode(Exp* p, const string& m);
    ~ArrowAccessNode();
    void accept(Visitor* visitor);

    void computeAddress(Visitor* visitor);
};

// Asignación dinámica: malloc(expr)
class MallocNode : public Exp {
public:
    Exp* size;
    MallocNode(Exp* s);
    ~MallocNode();
    void accept(Visitor* visitor);

};



// sizeof(tipo)
class SizeOfNode : public Exp {
public:
    Exp* target_type;
    SizeOfNode(Exp* t);
    ~SizeOfNode();
    void accept(Visitor* visitor);

};

// Referencia a variable por nombre
class IdentifierNode : public Exp {
public:
    string name;
    VarDecl* binding = nullptr;  // VarDecl resuelto por TypeChecker (scope/shadowing)
    IdentifierNode(const string& n);
    ~IdentifierNode();
    void accept(Visitor* visitor);

    void computeAddress(Visitor* visitor);
};

// Literal entero
class IntegerLiteralNode : public Exp {
public:
    long long value;
    IntegerLiteralNode(long long v);
    void accept(Visitor* visitor);

};

// Literal flotante
class FloatLiteralNode : public Exp {
public:
    double value;
    FloatLiteralNode(double v);
    void accept(Visitor* visitor);

};

// Literal booleano (true/false)
class BoolLiteralNode : public Exp {
public:
    bool value;
    BoolLiteralNode(bool v);
    void accept(Visitor* visitor);

};

// Literal de caracter: 'a'
class CharLiteralNode : public Exp {
public:
    char value;
    CharLiteralNode(char v);
    void accept(Visitor* visitor);

};

// Literal de cadena: "texto"
class StringLiteralNode : public Exp {
public:
    string value;
    StringLiteralNode(const string& v);
    void accept(Visitor* visitor);

};

// Impresión: printf(args...), es exp porque se puede usar en cualquier lugar, en c es valido x = printf("xd");
class PrintfNode : public Exp {
public:
    string format;  // string de formato (opcional, "%ld" por defecto)
    vector<Exp*> args;
    PrintfNode();
    PrintfNode(const string& fmt, const vector<Exp*>& a);
    ~PrintfNode();
    void accept(Visitor* visitor);

};

// Clase base para todos los statements
class Stm {
public:
    Location loc;
    Stm() {}
    virtual ~Stm() = 0;
    virtual void accept(Visitor* visitor) = 0;
};

// Bloque de código: { stmts... }
class Body : public Stm {
public:
    vector<Stm*> stmts;
    Body();
    ~Body();
    void accept(Visitor* visitor);

};

// Statement expresión: expr; | wrapper para poder usar nodes como stmts 
class ExprStmtNode : public Stm {
public:
    Exp* expr;
    ExprStmtNode(Exp* e);
    ~ExprStmtNode();
    void accept(Visitor* visitor);

};

// Condicional: if (cond) then [else]
class IfStmt : public Stm {
public:
    Exp* condition;
    Stm* then_branch;
    Stm* else_branch;
    IfStmt(Exp* c, Stm* t, Stm* e = nullptr);
    ~IfStmt();
    void accept(Visitor* visitor);

};

// Bucle while: while (cond) body
class WhileStmt : public Stm {
public:
    Exp* condition;
    Stm* body;
    WhileStmt(Exp* c, Stm* b);
    ~WhileStmt();
    void accept(Visitor* visitor);

};

// Bucle do-while: do body while (cond)
class DoWhileStmt : public Stm {
public:
    Stm* body;
    Exp* condition;
    DoWhileStmt(Stm* b, Exp* c);
    ~DoWhileStmt();
    void accept(Visitor* visitor);

};

// Bucle for: for (init; cond; inc) body
class ForStmt : public Stm {
public:
    Stm* init;
    Exp* condition;
    Exp* increment;
    Stm* body;
    ForStmt(Stm* i, Exp* c, Exp* inc, Stm* b);
    ~ForStmt();
    void accept(Visitor* visitor);

};

// Caso de switch: case value: body
class CaseClause : public Stm {
public:
    Exp* value;
    vector<Stm*> body;
    CaseClause(Exp* v);
    ~CaseClause();
    void accept(Visitor* visitor);

};

// Switch: switch (expr) { cases... [default: body] }
class SwitchStmt : public Stm {
public:
    Exp* expr;
    vector<CaseClause*> cases;
    vector<Stm*> default_body;
    SwitchStmt(Exp* e);
    ~SwitchStmt();
    void accept(Visitor* visitor);

};

// break
class BreakStmt : public Stm {
public:
    BreakStmt();
    void accept(Visitor* visitor);

};

// continue
class ContinueStmt : public Stm {
public:
    ContinueStmt();
    void accept(Visitor* visitor);

};

// return [expr]
class ReturnStmt : public Stm {
public:
    Exp* expr;
    ReturnStmt(Exp* e = nullptr);
    ~ReturnStmt();
    void accept(Visitor* visitor);

};

// free(expr)
class FreeStmt : public Stm {
public:
    Exp* expr;
    FreeStmt(Exp* e);
    ~FreeStmt();
    void accept(Visitor* visitor);

};

// --- Nodos de declaración ---

// Declaración de variable (hereda de Stm para ir directo en Body::stmts)
class VarDecl : public Stm {
public:
    Exp* type;
    string name;
    vector<Exp*> array_sizes; // para tamaños en arreglos [size1][size2]...
    Exp* initializer; // expresion inicial x = init;
    vector<Exp*> init_list; // lista de inicialización: = { e1, e2, ... }
    
    // Calculados por TypeChecker
    Type* resolvedType = nullptr;  // tipo semántico resuelto
    int offset = 0;      // offset desde %rbp
    int memSize = 0;     // tamaño en bytes

    VarDecl(Exp* t, const string& n);
    ~VarDecl();
    void accept(Visitor* visitor);

};

// Declaración de función
class FunDecl {
public:
    Location loc;
    Exp* return_type;
    string name;
    vector<VarDecl*> params;
    Body* body;
    
    // Calculados por TypeChecker
    int frameSize = 0;   // tamaño total del frame

    FunDecl(Exp* rt, const string& n, Body* b);
    ~FunDecl();
    void accept(Visitor* visitor);

};

// Declaración de struct
class StructDecl {
public:
    Location loc;
    string name;
    vector<VarDecl*> members;
    
    // Calculados por TypeChecker
    unordered_map<string, int> memberOffsets;  // offset de cada miembro
    unordered_map<string, int> memberSizes;    // size of each member in bytes
    int totalSize = 0;                          // tamaño total del struct

    StructDecl(const string& n);
    ~StructDecl();
    void accept(Visitor* visitor);

};

// Programa completo: lista de funciones, globales y structs
class Program {
public:
    Location loc;
    vector<FunDecl*> functions;
    vector<VarDecl*> globals;
    vector<StructDecl*> structs;

    Program();
    ~Program();
    void accept(Visitor* visitor);

};

// --- Nodos de tipo ---

// Clase base para nodos de tipo
class TypeNode : public Exp {
public:
    TypeNode() : Exp() {}
};

// Tipo primitivo: void, int, char, float, double, bool, long
class PrimitiveTypeNode : public TypeNode {
public:
    enum Prim { VOID, INT, CHAR, FLOAT, DOUBLE, BOOL, LONG };
    Prim prim;
    PrimitiveTypeNode(Prim p);
    void accept(Visitor* visitor);

};

// Tipo puntero: T*
class PointerTypeNode : public TypeNode {
public:
    TypeNode* base;
    PointerTypeNode(TypeNode* b);
    ~PointerTypeNode();
    void accept(Visitor* visitor);

};

// Tipo struct por nombre, instanciar un struct: struct Point p;
class StructTypeNode : public TypeNode {
public:
    string name;
    StructTypeNode(const string& n);
    void accept(Visitor* visitor);

};

#endif
