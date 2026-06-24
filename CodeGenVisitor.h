#ifndef CODEGENVISITOR_H
#define CODEGENVISITOR_H

class Exp;
class BinaryOpNode;
class UnaryOpNode;
class AssignmentNode;
class TernaryOpNode;
class FcallNode;
class IndexNode;
class MemberAccessNode;
class ArrowAccessNode;
class MallocNode;
class CastNode;
class SizeOfNode;
class IdentifierNode;
class IntegerLiteralNode;
class FloatLiteralNode;
class BoolLiteralNode;
class CharLiteralNode;
class StringLiteralNode;
class ParenthesizedExprNode;
class PrimitiveTypeNode;
class PointerTypeNode;
class StructTypeNode;
class NamedTypeNode;
class CaptureNode;
class LambdaExprNode;

class Stm;
class Body;
class ExprStmtNode;
class DeclStmt;
class IfStmt;
class WhileStmt;
class DoWhileStmt;
class ForStmt;
class SwitchStmt;
class CaseClause;
class DefaultClause;
class BreakStmt;
class ContinueStmt;
class ReturnStmt;
class FreeStmt;

class VarDecl;
class FunDecl;
class StructDecl;
class Program;
class TemplateDecl;

class CodeGenVisitor {
public:
    virtual ~CodeGenVisitor() = default;

    // --- Expressions ---
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
    virtual void visit(CaptureNode* e) = 0;
    virtual void visit(LambdaExprNode* e) = 0;

    // --- Statements ---
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

    // --- Declarations ---
    virtual void visit(VarDecl* d) = 0;
    virtual void visit(FunDecl* d) = 0;
    virtual void visit(StructDecl* d) = 0;
    virtual void visit(Program* p) = 0;
    virtual void visit(TemplateDecl* d) = 0;

    // --- Lvalue address computation ---
    virtual void computeAddress(UnaryOpNode* e) = 0;
    virtual void computeAddress(IdentifierNode* e) = 0;
    virtual void computeAddress(IndexNode* e) = 0;
    virtual void computeAddress(MemberAccessNode* e) = 0;
    virtual void computeAddress(ArrowAccessNode* e) = 0;
};

#endif
