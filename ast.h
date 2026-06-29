#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include "semantic_types.h"

using namespace std;

class Visitor;
class TypeVisitor;
class CodeGenVisitor;
class VarDecl;
class TemplateDecl;
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
    LOG_AND, LOG_OR, POW, COMMA
};

enum class UnaryOp {
    ADDR, DEREF, MINUS, LOG_NOT, PRE_INC, PRE_DEC, POST_INC, POST_DEC
};

enum class AssignOp {
    ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN
};

// Clase base para todos los nodos de expresión
class Exp {
public:
    Location loc;
    bool isConstant = false;
    double constantValue = 0.0;
    
    Exp() {}
    virtual ~Exp() {}
    virtual double accept(Visitor* visitor);
    virtual Type* accept(TypeVisitor* visitor);
    virtual void accept(CodeGenVisitor* visitor);
    virtual void computeAddress(CodeGenVisitor* visitor);
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
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Operación unaria (*p, -x, !b, ++i, etc.)
class UnaryOpNode : public Exp {
public:
    UnaryOp op;
    Exp* operand;
    UnaryOpNode(Exp* o, UnaryOp op);
    ~UnaryOpNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
    void computeAddress(CodeGenVisitor* visitor);
};

// Asignación: =, +=, -=, *=, /=
class AssignmentNode : public Exp {
public:
    AssignOp op;
    Exp* target;
    Exp* value;
    AssignmentNode(Exp* t, Exp* v, AssignOp op = AssignOp::ASSIGN);
    ~AssignmentNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Operador ternario: cond ? then : else
class TernaryOpNode : public Exp {
public:
    Exp* condition;
    Exp* then_expr;
    Exp* else_expr;
    TernaryOpNode(Exp* c, Exp* t, Exp* e);
    ~TernaryOpNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Llamada a función: callee(args...)
class FcallNode : public Exp {
public:
    Exp* callee; // nombre
    vector<Exp*> args; // argumentos que recibe la llamada
    vector<TypeNode*> template_args; // argumentos del template <arg, arg2...>
    FcallNode(Exp* c);
    ~FcallNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Acceso por índice: base[index]
class IndexNode : public Exp {
public:
    Exp* base;
    Exp* index;
    IndexNode(Exp* b, Exp* i);
    ~IndexNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
    void computeAddress(CodeGenVisitor* visitor);
};

// Acceso a miembro de struct: obj.member
class MemberAccessNode : public Exp {
public:
    Exp* object;
    string member;
    MemberAccessNode(Exp* o, const string& m);
    ~MemberAccessNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
    void computeAddress(CodeGenVisitor* visitor);
};

// Acceso a miembro por puntero: ptr->member
class ArrowAccessNode : public Exp {
public:
    Exp* pointer;
    string member;
    ArrowAccessNode(Exp* p, const string& m);
    ~ArrowAccessNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
    void computeAddress(CodeGenVisitor* visitor);
};

// Asignación dinámica: malloc(expr)
class MallocNode : public Exp {
public:
    Exp* size;
    MallocNode(Exp* s);
    ~MallocNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Conversión de tipo: (target_type)expr
class CastNode : public Exp {
public:
    Exp* target_type;
    Exp* expr;
    CastNode(Exp* t, Exp* e);
    ~CastNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// sizeof(tipo)
class SizeOfNode : public Exp {
public:
    Exp* target_type;
    SizeOfNode(Exp* t);
    ~SizeOfNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Referencia a variable por nombre
class IdentifierNode : public Exp {
public:
    string name;
    IdentifierNode(const string& n);
    ~IdentifierNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
    void computeAddress(CodeGenVisitor* visitor);
};

// Literal entero
class IntegerLiteralNode : public Exp {
public:
    long long value;
    IntegerLiteralNode(long long v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Literal flotante
class FloatLiteralNode : public Exp {
public:
    double value;
    FloatLiteralNode(double v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Literal booleano (true/false)
class BoolLiteralNode : public Exp {
public:
    bool value;
    BoolLiteralNode(bool v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Literal de caracter: 'a'
class CharLiteralNode : public Exp {
public:
    char value;
    CharLiteralNode(char v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Literal de cadena: "texto"
class StringLiteralNode : public Exp {
public:
    string value;
    StringLiteralNode(const string& v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Impresión: printf(args...), es exp porque se puede usar en cualquier lugar, en c es valido x = printf("xd");
class PrintfNode : public Exp {
public:
    vector<Exp*> args;
    PrintfNode();
    ~PrintfNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Clase base para todos los statements
class Stm {
public:
    Location loc;
    Stm() {}
    virtual ~Stm() = 0;
    virtual int accept(Visitor* visitor) = 0;
    virtual void accept(TypeVisitor* visitor) = 0;
    virtual void accept(CodeGenVisitor* visitor) = 0;
};

// Bloque de código: { stmts... }
class Body : public Stm {
public:
    vector<Stm*> stmts;
    Body();
    ~Body();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Statement expresión: expr; | wrapper para poder usar nodes como stmts 
class ExprStmtNode : public Stm {
public:
    Exp* expr;
    ExprStmtNode(Exp* e);
    ~ExprStmtNode();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Condicional: if (cond) then [else]
class IfStmt : public Stm {
public:
    Exp* condition;
    Stm* then_branch;
    Stm* else_branch;
    IfStmt(Exp* c, Stm* t, Stm* e = nullptr);
    ~IfStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Bucle while: while (cond) body
class WhileStmt : public Stm {
public:
    Exp* condition;
    Stm* body;
    WhileStmt(Exp* c, Stm* b);
    ~WhileStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Bucle do-while: do body while (cond)
class DoWhileStmt : public Stm {
public:
    Stm* body;
    Exp* condition;
    DoWhileStmt(Stm* b, Exp* c);
    ~DoWhileStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
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
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Caso de switch: case value: body
class CaseClause : public Stm {
public:
    Exp* value;
    vector<Stm*> body;
    CaseClause(Exp* v);
    ~CaseClause();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Switch: switch (expr) { cases... [default: body] }
class SwitchStmt : public Stm {
public:
    Exp* expr;
    vector<CaseClause*> cases;
    vector<Stm*> default_body;
    SwitchStmt(Exp* e);
    ~SwitchStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// break
class BreakStmt : public Stm {
public:
    BreakStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// continue
class ContinueStmt : public Stm {
public:
    ContinueStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// return [expr]
class ReturnStmt : public Stm {
public:
    Exp* expr;
    ReturnStmt(Exp* e = nullptr);
    ~ReturnStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// free(expr)
class FreeStmt : public Stm {
public:
    Exp* expr;
    FreeStmt(Exp* e);
    ~FreeStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// --- Nodos de declaración ---

// Declaración de variable (hereda de Stm para ir directo en Body::stmts)
class VarDecl : public Stm {
public:
    Exp* type;
    string name;
    vector<Exp*> array_sizes; // para tamaños en arreglos [size1][size2]...
    Exp* initializer; // expresion inicial x = init;
    
    // Calculados por TypeChecker
    Type* resolvedType = nullptr;  // tipo semántico resuelto
    int offset = 0;      // offset desde %rbp
    int memSize = 0;     // tamaño en bytes

    VarDecl(Exp* t, const string& n);
    ~VarDecl();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Declaración de función
class FunDecl {
public:
    Location loc;
    Exp* return_type;
    string name;
    vector<VarDecl*> params;
    Body* body;
    bool is_template;
    
    // Calculados por TypeChecker
    int frameSize = 0;   // tamaño total del frame

    FunDecl(Exp* rt, const string& n, Body* b);
    ~FunDecl();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Declaración de struct
class StructDecl {
public:
    Location loc;
    string name;
    vector<VarDecl*> members;
    
    // Calculados por TypeChecker
    unordered_map<string, int> memberOffsets;  // offset de cada miembro
    int totalSize = 0;                          // tamaño total del struct

    StructDecl(const string& n);
    ~StructDecl();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Programa completo: lista de funciones, globales, structs y templates
class Program {
public:
    Location loc;
    vector<FunDecl*> functions;
    vector<VarDecl*> globals;
    vector<StructDecl*> structs;
    vector<TemplateDecl*> templates;

    Program();
    ~Program();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// --- Nodos de tipo ---

// Clase base para nodos de tipo
class TypeNode : public Exp {
public:
    TypeNode() : Exp() {}
};

// Tipo primitivo: void, int, char, float, double, bool, auto
class PrimitiveTypeNode : public TypeNode {
public:
    enum Prim { VOID, INT, CHAR, FLOAT, DOUBLE, BOOL, AUTO };
    Prim prim;
    PrimitiveTypeNode(Prim p);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Tipo puntero: T*
class PointerTypeNode : public TypeNode {
public:
    TypeNode* base;
    PointerTypeNode(TypeNode* b);
    ~PointerTypeNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Tipo struct por nombre, instanciar un struct: struct Point p;
class StructTypeNode : public TypeNode {
public:
    string name;
    StructTypeNode(const string& n);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Tipo nombrado, nodos que representan tipos definidos por el usuario, por ejemplo en templates
class NamedTypeNode : public TypeNode {
public:
    string name;
    NamedTypeNode(const string& n);

    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// instanciacion concreta de template: Nombre<T1, T2, ...>
class TemplateTypeNode : public TypeNode {
public:
    string name;
    vector<TypeNode*> type_args;
    TemplateTypeNode(const string& n, const vector<TypeNode*>& args);
    ~TemplateTypeNode();

    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
    double accept(Visitor* visitor);
};

// --- Expresiones lambda ---

// Captura de variable en lambda: [&x] o [x]
class CaptureNode : public Exp {
public:
    enum Mode { BY_VALUE, BY_REF };
    Mode mode;
    string name;
    CaptureNode(Mode m, const string& n);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// Expresión lambda: [captures](params) -> tipo { body }
class LambdaExprNode : public Exp {
public:
    vector<CaptureNode*> captures;
    vector<VarDecl*> params;
    TypeNode* return_type;
    Body* body;
    LambdaExprNode(const vector<CaptureNode*>& caps, const vector<VarDecl*>& p, TypeNode* r, Body* b);
    ~LambdaExprNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// --- Declaraciones template ---

// Declaración template: template<params> func/struct
class TemplateDecl {
public:
    vector<string> params;
    FunDecl* func;
    StructDecl* struct_decl;
    bool is_function;
    TemplateDecl(const vector<string>& p, FunDecl* f);
    TemplateDecl(const vector<string>& p, StructDecl* s);
    ~TemplateDecl();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

#endif
