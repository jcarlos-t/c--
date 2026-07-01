#include "ast.h"
#include "visitor.h"

using namespace std;

// ============================================================
// accept() dispatch stubs for Visitor
// ============================================================
void BinaryOpNode::accept(Visitor* v) { v->visit(this); }
void UnaryOpNode::accept(Visitor* v) { v->visit(this); }
void AssignmentNode::accept(Visitor* v) { v->visit(this); }
void FcallNode::accept(Visitor* v) { v->visit(this); }
void MallocNode::accept(Visitor* v) { v->visit(this); }
void IndexNode::accept(Visitor* v) { v->visit(this); }
void MemberAccessNode::accept(Visitor* v) { v->visit(this); }
void ArrowAccessNode::accept(Visitor* v) { v->visit(this); }
void SizeOfNode::accept(Visitor* v) { v->visit(this); }
void LambdaExprNode::accept(Visitor* v) { v->visit(this); }
void CaptureNode::accept(Visitor* v) { v->visit(this); }
void IdentifierNode::accept(Visitor* v) { v->visit(this); }
void IntegerLiteralNode::accept(Visitor* v) { v->visit(this); }
void FloatLiteralNode::accept(Visitor* v) { v->visit(this); }
void BoolLiteralNode::accept(Visitor* v) { v->visit(this); }
void CharLiteralNode::accept(Visitor* v) { v->visit(this); }
void StringLiteralNode::accept(Visitor* v) { v->visit(this); }
void PrintfNode::accept(Visitor* v) { v->visit(this); }
void PrimitiveTypeNode::accept(Visitor* v) { v->visit(this); }
void PointerTypeNode::accept(Visitor* v) { v->visit(this); }
void StructTypeNode::accept(Visitor* v) { v->visit(this); }
void NamedTypeNode::accept(Visitor* v) { v->visit(this); }
void TemplateTypeNode::accept(Visitor* v) { v->visit(this); }
void Body::accept(Visitor* v) { v->visit(this); }
void ExprStmtNode::accept(Visitor* v) { v->visit(this); }
void IfStmt::accept(Visitor* v) { v->visit(this); }
void WhileStmt::accept(Visitor* v) { v->visit(this); }
void DoWhileStmt::accept(Visitor* v) { v->visit(this); }
void ForStmt::accept(Visitor* v) { v->visit(this); }
void SwitchStmt::accept(Visitor* v) { v->visit(this); }
void CaseClause::accept(Visitor* v) { v->visit(this); }
void BreakStmt::accept(Visitor* v) { v->visit(this); }
void ContinueStmt::accept(Visitor* v) { v->visit(this); }
void ReturnStmt::accept(Visitor* v) { v->visit(this); }
void FreeStmt::accept(Visitor* v) { v->visit(this); }
void VarDecl::accept(Visitor* v) { v->visit(this); }
void FunDecl::accept(Visitor* v) { v->visit(this); }
void StructDecl::accept(Visitor* v) { v->visit(this); }
void TemplateDecl::accept(Visitor* v) { v->visit(this); }
void Program::accept(Visitor* v) { v->visit(this); }
