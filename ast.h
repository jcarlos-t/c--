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
class DeclStmt;
class TemplateDecl;
class TypeNode;

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
    FunctionDecl, VariableDecl, StructDecl, TemplateDecl,
    PrimitiveType, PointerType, StructType, NamedType,
    Body, ExprStmt,
    IfStmt, WhileStmt, DoWhileStmt, ForStmt,
    SwitchStmt, CaseClause, DefaultClause,
    BreakStmt, ContinueStmt, ReturnStmt, FreeStmt,
    BinaryOp, UnaryOp, Assignment, TernaryOp, Call, Malloc,
    Subscript, MemberAccess, ArrowAccess,
    PostInc, PostDec, PreInc, PreDec, Cast, SizeOf,
    Printf,
    Identifier, LambdaExpr, Capture,
    IntegerLiteral, FloatLiteral, BoolLiteral,
    CharLiteral, StringLiteral, ParenthesizedExpr,
    Parameter, TemplateParam
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
// Clase base Exp 
// ============================================================
class Exp {
public:
    NodeKind kind;
    Location loc;
    Exp(NodeKind k) : kind(k) {}
    virtual ~Exp() = 0;
    virtual double accept(Visitor* visitor) = 0;
    virtual Type* accept(TypeVisitor* visitor) = 0;
    virtual void accept(CodeGenVisitor* visitor) = 0;
    virtual void computeAddress(CodeGenVisitor* visitor);
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
    void accept(CodeGenVisitor* visitor);
};

class UnaryOpNode : public Exp {
public:
    ::UnaryOp op;
    Exp* operand;
    UnaryOpNode(Exp* o, ::UnaryOp op);
    ~UnaryOpNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
    void computeAddress(CodeGenVisitor* visitor);
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
    void accept(CodeGenVisitor* visitor);
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
    void accept(CodeGenVisitor* visitor);
};

class FcallNode : public Exp {
public:
    Exp* callee;
    vector<Exp*> args;
    vector<TypeNode*> template_args;
    FcallNode(Exp* c) : Exp(NodeKind::Call), callee(c) {}
    ~FcallNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

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

class MallocNode : public Exp {
public:
    Exp* size;
    MallocNode(Exp* s);
    ~MallocNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

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

class SizeOfNode : public Exp {
public:
    Exp* target_type;
    SizeOfNode(Exp* t);
    ~SizeOfNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

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

class IntegerLiteralNode : public Exp {
public:
    long long value;
    IntegerLiteralNode(long long v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class FloatLiteralNode : public Exp {
public:
    double value;
    FloatLiteralNode(double v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class BoolLiteralNode : public Exp {
public:
    bool value;
    BoolLiteralNode(bool v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class CharLiteralNode : public Exp {
public:
    char value;
    CharLiteralNode(char v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class StringLiteralNode : public Exp {
public:
    string value;
    StringLiteralNode(const string& v);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class ParenthesizedExprNode : public Exp {
public:
    Exp* expr;
    ParenthesizedExprNode(Exp* e);
    ~ParenthesizedExprNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

// ============================================================
// Printf (grammar.md §8)
// ============================================================

class PrintfNode : public Exp {
public:
    vector<Exp*> args;
    PrintfNode();
    ~PrintfNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
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
    virtual void accept(CodeGenVisitor* visitor) = 0;
};

class Body : public Stm {
public:
    vector<Stm*> stmts;
    vector<VarDecl*> vdlist;
    Body();
    ~Body();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class ExprStmtNode : public Stm {
public:
    Exp* expr;
    ExprStmtNode(Exp* e);
    ~ExprStmtNode();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class DeclStmt : public Stm {
public:
    VarDecl* decl;
    DeclStmt(VarDecl* d);
    ~DeclStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
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
    void accept(CodeGenVisitor* visitor);
};

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

class SwitchStmt : public Stm {
public:
    Exp* expr;
    vector<Stm*> cases;
    SwitchStmt(Exp* e);
    ~SwitchStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

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

class DefaultClause : public Stm {
public:
    vector<Stm*> body;
    DefaultClause();
    ~DefaultClause();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class BreakStmt : public Stm {
public:
    BreakStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class ContinueStmt : public Stm {
public:
    ContinueStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class ReturnStmt : public Stm {
public:
    Exp* expr;
    ReturnStmt(Exp* e = nullptr);
    ~ReturnStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class FreeStmt : public Stm {
public:
    Exp* expr;
    FreeStmt(Exp* e);
    ~FreeStmt();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
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
    void accept(CodeGenVisitor* visitor);
};

class FunDecl {
public:
    NodeKind kind;
    Location loc;
    Exp* return_type;
    string name;
    vector<VarDecl*> params;
    Body* body;
    bool is_template;

    FunDecl(Exp* rt, const string& n, Body* b);
    ~FunDecl();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
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
    void accept(CodeGenVisitor* visitor);
};

class Program {
public:
    NodeKind kind;
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

// ============================================================
// Type nodes (ast.md §3)
// ============================================================
class TypeNode : public Exp {
public:
    TypeNode(NodeKind k) : Exp(k) {}
};

class PrimitiveTypeNode : public TypeNode {
public:
    enum Prim { VOID, INT, CHAR, FLOAT, DOUBLE, BOOL, AUTO };
    Prim prim;
    PrimitiveTypeNode(Prim p);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class PointerTypeNode : public TypeNode {
public:
    TypeNode* base;
    PointerTypeNode(TypeNode* b);
    ~PointerTypeNode();
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class StructTypeNode : public TypeNode {
public:
    string name;
    StructTypeNode(const string& n);
    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

class NamedTypeNode : public TypeNode {
public:
    string name;
    NamedTypeNode(const string& n);

    double accept(Visitor* visitor);
    Type* accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

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

// ============================================================
// Lambda expressions (grammar.md §8)
// ============================================================

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

// ============================================================
// Template declarations (grammar.md §10)
// ============================================================

class TemplateParam {
public:
    NodeKind kind;
    string name;
    TemplateParam(const string& n);
};

class TemplateDecl {
public:
    NodeKind kind;
    vector<TemplateParam*> params;
    FunDecl* func;
    StructDecl* struct_decl;
    bool is_function;
    TemplateDecl(const vector<TemplateParam*>& p, FunDecl* f);
    TemplateDecl(const vector<TemplateParam*>& p, StructDecl* s);
    ~TemplateDecl();
    int accept(Visitor* visitor);
    void accept(TypeVisitor* visitor);
    void accept(CodeGenVisitor* visitor);
};

#endif
