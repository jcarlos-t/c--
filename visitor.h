#ifndef VISITOR_H
#define VISITOR_H

#include "ast.h"
#include "environment.h"
#include "semantic_types.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <ostream>
#include <exception>

using namespace std;

#include <functional>

// ============================================================
// Control flow exceptions (used by EVALVisitor)
// ============================================================
class BreakException : public exception {};
class ContinueException : public exception {};
class ReturnException : public exception {
public:
    double value;
    ReturnException(double v) : value(v) {}
};

// ============================================================
// Abstract Visitor — evaluation (EVALVisitor)
// ============================================================
class Visitor {
public:
    virtual ~Visitor() = default;
    virtual double visit(BinaryOpNode* e) = 0;
    virtual double visit(UnaryOpNode* e) = 0;
    virtual double visit(AssignmentNode* e) = 0;
    virtual double visit(TernaryOpNode* e) = 0;
    virtual double visit(FcallNode* e) = 0;
    virtual double visit(MallocNode* e) = 0;
    virtual double visit(IndexNode* e) = 0;
    virtual double visit(MemberAccessNode* e) = 0;
    virtual double visit(ArrowAccessNode* e) = 0;
    virtual double visit(CastNode* e) = 0;
    virtual double visit(SizeOfNode* e) = 0;
    virtual double visit(LambdaExprNode* e) = 0;
    virtual double visit(CaptureNode* e) = 0;
    virtual double visit(IdentifierNode* e) = 0;
    virtual double visit(IntegerLiteralNode* e) = 0;
    virtual double visit(FloatLiteralNode* e) = 0;
    virtual double visit(BoolLiteralNode* e) = 0;
    virtual double visit(CharLiteralNode* e) = 0;
    virtual double visit(StringLiteralNode* e) = 0;
    virtual double visit(ParenthesizedExprNode* e) = 0;
    virtual double visit(PrimitiveTypeNode* e) = 0;
    virtual double visit(PointerTypeNode* e) = 0;
    virtual double visit(StructTypeNode* e) = 0;
    virtual double visit(NamedTypeNode* e) = 0;
    virtual double visit(TemplateTypeNode* e) = 0;

    virtual int visit(Body* s) = 0;
    virtual int visit(ExprStmtNode* s) = 0;
    virtual int visit(DeclStmt* s) = 0;
    virtual int visit(IfStmt* s) = 0;
    virtual int visit(WhileStmt* s) = 0;
    virtual int visit(DoWhileStmt* s) = 0;
    virtual int visit(ForStmt* s) = 0;
    virtual int visit(SwitchStmt* s) = 0;
    virtual int visit(CaseClause* s) = 0;
    virtual int visit(DefaultClause* s) = 0;
    virtual int visit(BreakStmt* s) = 0;
    virtual int visit(ContinueStmt* s) = 0;
    virtual int visit(ReturnStmt* s) = 0;
    virtual int visit(FreeStmt* s) = 0;

    virtual int visit(VarDecl* d) = 0;
    virtual int visit(FunDecl* d) = 0;
    virtual int visit(StructDecl* d) = 0;
    virtual int visit(TemplateDecl* d) = 0;
    virtual int visit(Program* p) = 0;
};

// ============================================================
// EVALVisitor — tree-walking interpreter
// ============================================================
class EVALVisitor : public Visitor {
public:
    Environment<double> env;
    Environment<string> typeEnv;
    unordered_map<string, FunDecl*> envfun;
    unordered_map<string, string> funReturnTypes;
    string last_string;
    unordered_map<string, vector<double>> array_data;
    unordered_map<string, unordered_map<string, double>> struct_instances;
    unordered_map<string, vector<string>> struct_defs;
    unordered_map<int, vector<double>> heap;
    int next_addr = 1;

    double visit(BinaryOpNode* e) override;
    double visit(MallocNode* e) override;
    double visit(SizeOfNode* e) override;
    double visit(UnaryOpNode* e) override;
    double visit(AssignmentNode* e) override;
    double visit(TernaryOpNode* e) override;
    double visit(FcallNode* e) override;
    double visit(IndexNode* e) override;
    double visit(MemberAccessNode* e) override;
    double visit(ArrowAccessNode* e) override;
    double visit(CastNode* e) override;
    double visit(LambdaExprNode* e) override;
    double visit(CaptureNode* e) override;
    double visit(IdentifierNode* e) override;
    double visit(IntegerLiteralNode* e) override;
    double visit(FloatLiteralNode* e) override;
    double visit(BoolLiteralNode* e) override;
    double visit(CharLiteralNode* e) override;
    double visit(StringLiteralNode* e) override;
    double visit(ParenthesizedExprNode* e) override;
    double visit(PrimitiveTypeNode* e) override;
    double visit(PointerTypeNode* e) override;
    double visit(StructTypeNode* e) override;
    double visit(NamedTypeNode* e) override;
    double visit(TemplateTypeNode* e) override;

    int visit(Body* s) override;
    int visit(ExprStmtNode* s) override;
    int visit(DeclStmt* s) override;
    int visit(IfStmt* s) override;
    int visit(WhileStmt* s) override;
    int visit(DoWhileStmt* s) override;
    int visit(ForStmt* s) override;
    int visit(SwitchStmt* s) override;
    int visit(CaseClause* s) override;
    int visit(DefaultClause* s) override;
    int visit(BreakStmt* s) override;
    int visit(ContinueStmt* s) override;
    int visit(ReturnStmt* s) override;
    int visit(FreeStmt* s) override;

    int visit(VarDecl* d) override;
    int visit(FunDecl* d) override;
    int visit(StructDecl* d) override;
    int visit(TemplateDecl* d) override;
    int visit(Program* p) override;

    void interprete(Program* program);
    string getType(Exp* e);
};

// ============================================================
// Abstract TypeVisitor — semantic type checking
// ============================================================
class TypeVisitor {
public:
    virtual ~TypeVisitor() = default;
    virtual void visit(Program* p) = 0;
    virtual void visit(FunDecl* f) = 0;
    virtual void visit(VarDecl* v) = 0;
    virtual void visit(StructDecl* s) = 0;
    virtual void visit(TemplateDecl* d) = 0;
    virtual void visit(Body* b) = 0;
    virtual void visit(ExprStmtNode* s) = 0;
    virtual void visit(DeclStmt* s) = 0;
    virtual void visit(IfStmt* s) = 0;
    virtual void visit(WhileStmt* s) = 0;
    virtual void visit(DoWhileStmt* s) = 0;
    virtual void visit(ForStmt* s) = 0;
    virtual void visit(SwitchStmt* s) = 0;
    virtual void visit(CaseClause* s) = 0;
    virtual void visit(DefaultClause* s) = 0;
    virtual void visit(BreakStmt* s) = 0;
    virtual void visit(ContinueStmt* s) = 0;
    virtual void visit(ReturnStmt* s) = 0;
    virtual void visit(FreeStmt* s) = 0;

    virtual ::Type* visit(BinaryOpNode* e) = 0;
    virtual ::Type* visit(UnaryOpNode* e) = 0;
    virtual ::Type* visit(AssignmentNode* e) = 0;
    virtual ::Type* visit(TernaryOpNode* e) = 0;
    virtual ::Type* visit(FcallNode* e) = 0;
    virtual ::Type* visit(MallocNode* e) = 0;
    virtual ::Type* visit(IndexNode* e) = 0;
    virtual ::Type* visit(MemberAccessNode* e) = 0;
    virtual ::Type* visit(ArrowAccessNode* e) = 0;
    virtual ::Type* visit(CastNode* e) = 0;
    virtual ::Type* visit(SizeOfNode* e) = 0;
    virtual ::Type* visit(LambdaExprNode* e) = 0;
    virtual ::Type* visit(CaptureNode* e) = 0;
    virtual ::Type* visit(IdentifierNode* e) = 0;
    virtual ::Type* visit(IntegerLiteralNode* e) = 0;
    virtual ::Type* visit(FloatLiteralNode* e) = 0;
    virtual ::Type* visit(BoolLiteralNode* e) = 0;
    virtual ::Type* visit(CharLiteralNode* e) = 0;
    virtual ::Type* visit(StringLiteralNode* e) = 0;
    virtual ::Type* visit(ParenthesizedExprNode* e) = 0;
    virtual ::Type* visit(PrimitiveTypeNode* e) = 0;
    virtual ::Type* visit(PointerTypeNode* e) = 0;
    virtual ::Type* visit(StructTypeNode* e) = 0;
    virtual ::Type* visit(NamedTypeNode* e) = 0;
    virtual ::Type* visit(TemplateTypeNode* e) = 0;
};

// ============================================================
// FuncInfo — function signature for type checking
// ============================================================
struct FuncInfo {
    ::Type* returnType;
    vector<::Type*> paramTypes;
};

// ============================================================
// TypeChecker — semantic analysis concrete visitor
// ============================================================
class TypeChecker : public TypeVisitor {
private:
    Environment<::Type*> env;
    unordered_map<string, FuncInfo> functions;
    unordered_map<string, StructType*> struct_types;
    unordered_map<string, TemplateDecl*> template_decls;
    ::Type* retornodefuncion;
    ::Type* intType;
    ::Type* boolType;
    ::Type* voidType;
    ::Type* floatType;
    ::Type* charType;
    vector<::Type*> typeCache;
    int loopDepth;
    int switchDepth;
    vector<string> errors;
    bool hasError;

    void add_function(FunDecl* fd);
    ::Type* type_from_ast(Exp* t);
    bool check_assign(::Type* target, ::Type* value);
    void error(const string& msg);
    void error(const string& msg, const Location& loc);
    StructType* instantiate_template(const string& name, const vector<TypeNode*>& args);

public:
    TypeChecker();
    ~TypeChecker();
    void typecheck(Program* program);
    bool check(Program* program);

    void visit(Program* p) override;
    void visit(FunDecl* f) override;
    void visit(VarDecl* v) override;
    void visit(StructDecl* s) override;
    void visit(TemplateDecl* d) override;
    void visit(Body* b) override;
    void visit(ExprStmtNode* s) override;
    void visit(DeclStmt* s) override;
    void visit(IfStmt* s) override;
    void visit(WhileStmt* s) override;
    void visit(DoWhileStmt* s) override;
    void visit(ForStmt* s) override;
    void visit(SwitchStmt* s) override;
    void visit(CaseClause* s) override;
    void visit(DefaultClause* s) override;
    void visit(BreakStmt* s) override;
    void visit(ContinueStmt* s) override;
    void visit(ReturnStmt* s) override;
    void visit(FreeStmt* s) override;

    ::Type* visit(BinaryOpNode* e) override;
    ::Type* visit(UnaryOpNode* e) override;
    ::Type* visit(AssignmentNode* e) override;
    ::Type* visit(TernaryOpNode* e) override;
    ::Type* visit(FcallNode* e) override;
    ::Type* visit(MallocNode* e) override;
    ::Type* visit(IndexNode* e) override;
    ::Type* visit(MemberAccessNode* e) override;
    ::Type* visit(ArrowAccessNode* e) override;
    ::Type* visit(CastNode* e) override;
    ::Type* visit(SizeOfNode* e) override;
    ::Type* visit(LambdaExprNode* e) override;
    ::Type* visit(CaptureNode* e) override;
    ::Type* visit(IdentifierNode* e) override;
    ::Type* visit(IntegerLiteralNode* e) override;
    ::Type* visit(FloatLiteralNode* e) override;
    ::Type* visit(BoolLiteralNode* e) override;
    ::Type* visit(CharLiteralNode* e) override;
    ::Type* visit(StringLiteralNode* e) override;
    ::Type* visit(ParenthesizedExprNode* e) override;
    ::Type* visit(PrimitiveTypeNode* e) override;
    ::Type* visit(PointerTypeNode* e) override;
    ::Type* visit(StructTypeNode* e) override;
    ::Type* visit(NamedTypeNode* e) override;
    ::Type* visit(TemplateTypeNode* e) override;
};

// ============================================================
// Abstract CodeGenVisitor — code generation
// ============================================================
class CodeGenVisitor {
public:
    virtual ~CodeGenVisitor() = default;

    virtual void visit(BinaryOpNode* e) = 0;
    virtual void visit(UnaryOpNode* e) = 0;
    virtual void visit(AssignmentNode* e) = 0;
    virtual void visit(TernaryOpNode* e) = 0;
    virtual void visit(FcallNode* e) = 0;
    virtual void visit(IndexNode* e) = 0;
    virtual void visit(MemberAccessNode* e) = 0;
    virtual void visit(ArrowAccessNode* e) = 0;
    virtual void visit(MallocNode* e) = 0;
    virtual void visit(CastNode* e) = 0;
    virtual void visit(SizeOfNode* e) = 0;
    virtual void visit(IdentifierNode* e) = 0;
    virtual void visit(IntegerLiteralNode* e) = 0;
    virtual void visit(FloatLiteralNode* e) = 0;
    virtual void visit(BoolLiteralNode* e) = 0;
    virtual void visit(CharLiteralNode* e) = 0;
    virtual void visit(StringLiteralNode* e) = 0;
    virtual void visit(ParenthesizedExprNode* e) = 0;
    virtual void visit(PrimitiveTypeNode* e) = 0;
    virtual void visit(PointerTypeNode* e) = 0;
    virtual void visit(StructTypeNode* e) = 0;
    virtual void visit(NamedTypeNode* e) = 0;
    virtual void visit(TemplateTypeNode* e) = 0;
    virtual void visit(CaptureNode* e) = 0;
    virtual void visit(LambdaExprNode* e) = 0;

    virtual void visit(Body* s) = 0;
    virtual void visit(ExprStmtNode* s) = 0;
    virtual void visit(DeclStmt* s) = 0;
    virtual void visit(IfStmt* s) = 0;
    virtual void visit(WhileStmt* s) = 0;
    virtual void visit(DoWhileStmt* s) = 0;
    virtual void visit(ForStmt* s) = 0;
    virtual void visit(SwitchStmt* s) = 0;
    virtual void visit(CaseClause* s) = 0;
    virtual void visit(DefaultClause* s) = 0;
    virtual void visit(BreakStmt* s) = 0;
    virtual void visit(ContinueStmt* s) = 0;
    virtual void visit(ReturnStmt* s) = 0;
    virtual void visit(FreeStmt* s) = 0;

    virtual void visit(VarDecl* d) = 0;
    virtual void visit(FunDecl* d) = 0;
    virtual void visit(StructDecl* d) = 0;
    virtual void visit(Program* p) = 0;
    virtual void visit(TemplateDecl* d) = 0;

    virtual void computeAddress(UnaryOpNode* e) = 0;
    virtual void computeAddress(IdentifierNode* e) = 0;
    virtual void computeAddress(IndexNode* e) = 0;
    virtual void computeAddress(MemberAccessNode* e) = 0;
    virtual void computeAddress(ArrowAccessNode* e) = 0;
};

// ============================================================
// GenCodeVisitor — x86-64 AT&T code generator
// ============================================================
class GenCodeVisitor : public CodeGenVisitor {
public:
    enum class LValKind { Invalid, Id, Index, Member, Deref };
    struct LVal {
        LValKind kind = LValKind::Invalid;
        string name;
        Exp *index = nullptr;
        string member;
    };

private:
    ostream &out;
    LVal *lvalTarget = nullptr;
    LVal captureLVal(Exp *e);
    void storeTarget(const LVal &lv);

    unordered_map<string, int> memoria;
    unordered_map<string, bool> memoriaGlobal;
    unordered_map<string, int> arraySizes;
    unordered_map<string, int> structFieldCount;
    unordered_map<string, unordered_map<string, int>> structFieldOffset;

    int offset = -8;
    int labelcont = 0;
    bool inFunction = false;
    string currentBreakLabel;
    string funcName;

public:
    explicit GenCodeVisitor(ostream &out) : out(out) {}

    void generate(Program *p);

    void visit(BinaryOpNode *e) override;
    void visit(UnaryOpNode *e) override;
    void visit(AssignmentNode *e) override;
    void visit(TernaryOpNode *e) override;
    void visit(FcallNode *e) override;
    void visit(IndexNode *e) override;
    void visit(MemberAccessNode *e) override;
    void visit(ArrowAccessNode *e) override;
    void visit(MallocNode *e) override;
    void visit(CastNode *e) override;
    void visit(SizeOfNode *e) override;
    void visit(IdentifierNode *e) override;
    void visit(IntegerLiteralNode *e) override;
    void visit(FloatLiteralNode *e) override;
    void visit(BoolLiteralNode *e) override;
    void visit(CharLiteralNode *e) override;
    void visit(StringLiteralNode *e) override;
    void visit(ParenthesizedExprNode *e) override;
    void visit(PrimitiveTypeNode *e) override;
    void visit(PointerTypeNode *e) override;
    void visit(StructTypeNode *e) override;
    void visit(NamedTypeNode *e) override;
    void visit(TemplateTypeNode *e) override;
    void visit(CaptureNode *e) override;
    void visit(LambdaExprNode *e) override;

    void visit(Body *s) override;
    void visit(ExprStmtNode *s) override;
    void visit(DeclStmt *s) override;
    void visit(IfStmt *s) override;
    void visit(WhileStmt *s) override;
    void visit(DoWhileStmt *s) override;
    void visit(ForStmt *s) override;
    void visit(SwitchStmt *s) override;
    void visit(CaseClause *s) override;
    void visit(DefaultClause *s) override;
    void visit(BreakStmt *s) override;
    void visit(ContinueStmt *s) override;
    void visit(ReturnStmt *s) override;
    void visit(FreeStmt *s) override;

    void visit(VarDecl *d) override;
    void visit(FunDecl *d) override;
    void visit(StructDecl *d) override;
    void visit(Program *p) override;
    void visit(TemplateDecl *d) override;

    void computeAddress(UnaryOpNode *e) override;
    void computeAddress(IdentifierNode *e) override;
    void computeAddress(IndexNode *e) override;
    void computeAddress(MemberAccessNode *e) override;
    void computeAddress(ArrowAccessNode *e) override;
};

#endif
