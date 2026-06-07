#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include "semantic_types.h"

using namespace std;

class Visitor;
class TypeVisitor;
class VarDecl;

// ============================================================
// Location (ast.md §2)
// ============================================================
struct Location {
    int line;
    int column;
    Location(int l = 0, int c = 0) : line(l), column(c) {}
};

// ============================================================
// NodeKind (ast.md §2)
// ============================================================
enum class NodeKind {
    Program,
    FunctionDecl, VariableDecl, StructDecl,
    PrimitiveType, PointerType, StructType, NamedType,
    CompoundStmt, ExprStmt,
    IfStmt, WhileStmt, DoWhileStmt, ForStmt,
    SwitchStmt, CaseClause, DefaultClause,
    BreakStmt, ContinueStmt, ReturnStmt,
    BinaryOp, UnaryOp, Assignment, TernaryOp, Call,
    Subscript, MemberAccess, ArrowAccess,
    PostInc, PostDec, PreInc, PreDec, Cast,
    Identifier,
    IntegerLiteral, FloatLiteral, BoolLiteral,
    CharLiteral, StringLiteral, ParenthesizedExpr,
    Parameter
};

// ============================================================
// Operadores (ast.md §3)
// ============================================================
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

// ============================================================
// Clase base Exp (ast.md §3 — expressions)
// ============================================================
class Exp {
public:
    NodeKind kind;
    Location loc;
    Exp(NodeKind k) : kind(k) {}
    virtual ~Exp() = 0;
    virtual double accept(Visitor* visitor) = 0;
    virtual Type* accept(TypeVisitor* visitor) = 0;
};

// --- Expression nodes ---

class BinaryOpNode : public Exp {
public:
    ::BinaryOp op;
    Exp* left;
    Exp* right;
    BinaryOpNode(Exp* l, Exp* r, ::BinaryOp op);
    ~BinaryOpNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class UnaryOpNode : public Exp {
public:
    ::UnaryOp op;
    Exp* operand;
    UnaryOpNode(Exp* o, ::UnaryOp op);
    ~UnaryOpNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class AssignmentNode : public Exp {
public:
    AssignOp op;
    Exp* target;
    Exp* value;
    AssignmentNode(Exp* t, Exp* v, AssignOp op = AssignOp::ASSIGN);
    ~AssignmentNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class TernaryOpNode : public Exp {
public:
    Exp* condition;
    Exp* then_expr;
    Exp* else_expr;
    TernaryOpNode(Exp* c, Exp* t, Exp* e);
    ~TernaryOpNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class CallNode : public Exp {
public:
    Exp* callee;
    vector<Exp*> args;
    CallNode(Exp* c) : Exp(NodeKind::Call), callee(c) {}
    ~CallNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class SubscriptNode : public Exp {
public:
    Exp* base;
    Exp* index;
    SubscriptNode(Exp* b, Exp* i);
    ~SubscriptNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class MemberAccessNode : public Exp {
public:
    Exp* object;
    string member;
    MemberAccessNode(Exp* o, const string& m);
    ~MemberAccessNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class ArrowAccessNode : public Exp {
public:
    Exp* pointer;
    string member;
    ArrowAccessNode(Exp* p, const string& m);
    ~ArrowAccessNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class CastNode : public Exp {
public:
    Exp* target_type;
    Exp* expr;
    CastNode(Exp* t, Exp* e);
    ~CastNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class IdentifierNode : public Exp {
public:
    string name;
    IdentifierNode(const string& n);
    ~IdentifierNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class IntegerLiteralNode : public Exp {
public:
    long long value;
    IntegerLiteralNode(long long v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class FloatLiteralNode : public Exp {
public:
    double value;
    FloatLiteralNode(double v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class BoolLiteralNode : public Exp {
public:
    bool value;
    BoolLiteralNode(bool v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class CharLiteralNode : public Exp {
public:
    char value;
    CharLiteralNode(char v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class StringLiteralNode : public Exp {
public:
    string value;
    StringLiteralNode(const string& v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class ParenthesizedExprNode : public Exp {
public:
    Exp* expr;
    ParenthesizedExprNode(Exp* e);
    ~ParenthesizedExprNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

// ============================================================
// Clase base Stm (ast.md §3 — statements)
// ============================================================
class Stm {
public:
    NodeKind kind;
    Location loc;
    Stm(NodeKind k) : kind(k) {}
    virtual ~Stm() = 0;
    virtual int accept(Visitor* visitor) = 0;
    virtual void accept(TypeVisitor* visitor) = 0;
};

class CompoundStmt : public Stm {
public:
    vector<Stm*> stmts;
    vector<VarDecl*> vdlist;
    CompoundStmt();
    ~CompoundStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

class ExprStmtNode : public Stm {
public:
    Exp* expr;
    ExprStmtNode(Exp* e);
    ~ExprStmtNode();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

class IfStmt : public Stm {
public:
    Exp* condition;
    Stm* then_branch;
    Stm* else_branch;
    IfStmt(Exp* c, Stm* t, Stm* e = nullptr);
    ~IfStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

class WhileStmt : public Stm {
public:
    Exp* condition;
    Stm* body;
    WhileStmt(Exp* c, Stm* b);
    ~WhileStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

class DoWhileStmt : public Stm {
public:
    Stm* body;
    Exp* condition;
    DoWhileStmt(Stm* b, Exp* c);
    ~DoWhileStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

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
};

class SwitchStmt : public Stm {
public:
    Exp* expr;
    vector<Stm*> cases;
    SwitchStmt(Exp* e);
    ~SwitchStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

class BreakStmt : public Stm {
public:
    BreakStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

class ContinueStmt : public Stm {
public:
    ContinueStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

class ReturnStmt : public Stm {
public:
    Exp* expr;
    ReturnStmt(Exp* e = nullptr);
    ~ReturnStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

// ============================================================
// Declaration nodes (ast.md §3)
// ============================================================
class VarDecl {
public:
    NodeKind kind;
    Location loc;
    Exp* type;
    string name;
    vector<Exp*> array_sizes;
    Exp* initializer;

    VarDecl(Exp* t, const string& n);
    ~VarDecl();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

class FunDecl {
public:
    NodeKind kind;
    Location loc;
    Exp* return_type;
    string name;
    vector<VarDecl*> params;
    CompoundStmt* body;
    bool is_template;

    FunDecl(Exp* rt, const string& n, CompoundStmt* b);
    ~FunDecl();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

class StructDecl {
public:
    NodeKind kind;
    Location loc;
    string name;
    vector<VarDecl*> members;

    StructDecl(const string& n);
    ~StructDecl();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

class Program {
public:
    NodeKind kind;
    Location loc;
    vector<FunDecl*> functions;
    vector<VarDecl*> globals;
    vector<StructDecl*> structs;

    Program();
    ~Program();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
};

// ============================================================
// Type nodes (ast.md §3)
// ============================================================
class TypeNode : public Exp {
public:
    TypeNode(NodeKind k) : Exp(k) {}
    static TypeNode* from_basic(const string& name);
};

class PrimitiveTypeNode : public TypeNode {
public:
    enum Prim { VOID, INT, CHAR, FLOAT, DOUBLE, BOOL, AUTO };
    Prim prim;
    PrimitiveTypeNode(Prim p);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class PointerTypeNode : public TypeNode {
public:
    TypeNode* base;
    PointerTypeNode(TypeNode* b);
    ~PointerTypeNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class StructTypeNode : public TypeNode {
public:
    string name;
    StructTypeNode(const string& n);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

class NamedTypeNode : public TypeNode {
public:
    string name;
    NamedTypeNode(const string& n);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
};

#endif
