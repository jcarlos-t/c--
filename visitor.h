#ifndef VISITOR_H
#define VISITOR_H

#include "ast.h"
#include "environment.h"
#include <unordered_map>
#include <exception>

// Control flow exceptions for break/continue
class BreakException : public exception {};
class ContinueException : public exception {};
class ReturnException : public exception {
public:
    double value;
    ReturnException(double v) : value(v) {}
};

class Visitor {
public:
    virtual double visit(BinaryOpNode* e) = 0;
    virtual double visit(UnaryOpNode* e) = 0;
    virtual double visit(AssignmentNode* e) = 0;
    virtual double visit(TernaryOpNode* e) = 0;
    virtual double visit(CallNode* e) = 0;
    virtual double visit(MallocNode* e) = 0;
    virtual double visit(SubscriptNode* e) = 0;
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

    virtual int visit(CompoundStmt* s) = 0;
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

class PrintVisitor : public Visitor {
public:
    int indent;
    PrintVisitor() : indent(0) {}
    void print_indent();

    double visit(BinaryOpNode* e) override;
    double visit(MallocNode* e) override;
    double visit(SizeOfNode* e) override;
    double visit(UnaryOpNode* e) override;
    double visit(AssignmentNode* e) override;
    double visit(TernaryOpNode* e) override;
    double visit(CallNode* e) override;
    double visit(SubscriptNode* e) override;
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

    int visit(CompoundStmt* s) override;
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

    void imprimir(Program* program);
};

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
    double visit(CallNode* e) override;
    double visit(SubscriptNode* e) override;
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

    int visit(CompoundStmt* s) override;
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

#endif
