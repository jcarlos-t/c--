#include "ast.h"
#include "visitor.h"

using namespace std;

// ============================================================
// accept() methods for Visitor (double-returning)
// Expressions
// ============================================================
double BinaryOpNode::accept(Visitor* v) { return v->visit(this); }
double UnaryOpNode::accept(Visitor* v) { return v->visit(this); }
double AssignmentNode::accept(Visitor* v) { return v->visit(this); }
double FcallNode::accept(Visitor* v) { return v->visit(this); }
double MallocNode::accept(Visitor* v) { return v->visit(this); }
double IndexNode::accept(Visitor* v) { return v->visit(this); }
double MemberAccessNode::accept(Visitor* v) { return v->visit(this); }
double ArrowAccessNode::accept(Visitor* v) { return v->visit(this); }
double SizeOfNode::accept(Visitor* v) { return v->visit(this); }
double LambdaExprNode::accept(Visitor* v) { return v->visit(this); }
double CaptureNode::accept(Visitor* v) { return v->visit(this); }
double IdentifierNode::accept(Visitor* v) { return v->visit(this); }
double IntegerLiteralNode::accept(Visitor* v) { return v->visit(this); }
double FloatLiteralNode::accept(Visitor* v) { return v->visit(this); }
double BoolLiteralNode::accept(Visitor* v) { return v->visit(this); }
double CharLiteralNode::accept(Visitor* v) { return v->visit(this); }
double StringLiteralNode::accept(Visitor* v) { return v->visit(this); }
double PrintfNode::accept(Visitor* v) { return v->visit(this); }
double PrimitiveTypeNode::accept(Visitor* v) { return v->visit(this); }
double PointerTypeNode::accept(Visitor* v) { return v->visit(this); }
double StructTypeNode::accept(Visitor* v) { return v->visit(this); }
double NamedTypeNode::accept(Visitor* v) { return v->visit(this); }
double TemplateTypeNode::accept(Visitor* v) { return v->visit(this); }

// ============================================================
// accept() methods for Visitor (int-returning)
// Statements & Declarations
// ============================================================
int Body::accept(Visitor* v) { return v->visit(this); }
int ExprStmtNode::accept(Visitor* v) { return v->visit(this); }
int IfStmt::accept(Visitor* v) { return v->visit(this); }
int WhileStmt::accept(Visitor* v) { return v->visit(this); }
int DoWhileStmt::accept(Visitor* v) { return v->visit(this); }
int ForStmt::accept(Visitor* v) { return v->visit(this); }
int SwitchStmt::accept(Visitor* v) { return v->visit(this); }
int CaseClause::accept(Visitor* v) { return v->visit(this); }
int BreakStmt::accept(Visitor* v) { return v->visit(this); }
int ContinueStmt::accept(Visitor* v) { return v->visit(this); }
int ReturnStmt::accept(Visitor* v) { return v->visit(this); }
int FreeStmt::accept(Visitor* v) { return v->visit(this); }
int VarDecl::accept(Visitor* v) { return v->visit(this); }
int FunDecl::accept(Visitor* v) { return v->visit(this); }
int StructDecl::accept(Visitor* v) { return v->visit(this); }
int TemplateDecl::accept(Visitor* v) { return v->visit(this); }
int Program::accept(Visitor* v) { return v->visit(this); }
