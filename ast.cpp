#include "ast.h"
#include "CodeGenVisitor.h"
#include <iostream>

using namespace std;

// ===================== Exp base =====================
Exp::~Exp() {}

void Exp::computeAddress(CodeGenVisitor*) {
    cerr << "Error: el nodo no es un lvalue" << endl;
}

// ===================== BinaryOpNode =====================
BinaryOpNode::BinaryOpNode(Exp* l, Exp* r, ::BinaryOp o)
    : Exp(NodeKind::BinaryOp), left(l), right(r), op(o) {}
BinaryOpNode::~BinaryOpNode() { delete left; delete right; }

// ===================== UnaryOpNode =====================
UnaryOpNode::UnaryOpNode(Exp* o, ::UnaryOp uop)
    : Exp(NodeKind::UnaryOp), operand(o), op(uop) {}
UnaryOpNode::~UnaryOpNode() { delete operand; }

// ===================== AssignmentNode =====================
AssignmentNode::AssignmentNode(Exp* t, Exp* v, AssignOp o)
    : Exp(NodeKind::Assignment), target(t), value(v), op(o) {}
AssignmentNode::~AssignmentNode() { delete target; delete value; }

// ===================== TernaryOpNode =====================
TernaryOpNode::TernaryOpNode(Exp* c, Exp* t, Exp* e)
    : Exp(NodeKind::TernaryOp), condition(c), then_expr(t), else_expr(e) {}
TernaryOpNode::~TernaryOpNode() { delete condition; delete then_expr; delete else_expr; }

// ===================== FcallNode =====================
FcallNode::~FcallNode() { delete callee; for (auto a : args) delete a; }

// ===================== MallocNode =====================
MallocNode::MallocNode(Exp* s) : Exp(NodeKind::Malloc), size(s) {}
MallocNode::~MallocNode() { delete size; }

// ===================== IndexNode =====================
IndexNode::IndexNode(Exp* b, Exp* i)
    : Exp(NodeKind::Subscript), base(b), index(i) {}
IndexNode::~IndexNode() { delete base; delete index; }

// ===================== MemberAccessNode =====================
MemberAccessNode::MemberAccessNode(Exp* o, const string& m)
    : Exp(NodeKind::MemberAccess), object(o), member(m) {}
MemberAccessNode::~MemberAccessNode() { delete object; }

// ===================== ArrowAccessNode =====================
ArrowAccessNode::ArrowAccessNode(Exp* p, const string& m)
    : Exp(NodeKind::ArrowAccess), pointer(p), member(m) {}
ArrowAccessNode::~ArrowAccessNode() { delete pointer; }

// ===================== CastNode =====================
CastNode::CastNode(Exp* t, Exp* e)
    : Exp(NodeKind::Cast), target_type(t), expr(e) {}
CastNode::~CastNode() { delete target_type; delete expr; }

// ===================== SizeOfNode =====================
SizeOfNode::SizeOfNode(Exp* t) : Exp(NodeKind::SizeOf), target_type(t) {}
SizeOfNode::~SizeOfNode() { delete target_type; }

// ===================== IdentifierNode =====================
IdentifierNode::IdentifierNode(const string& n)
    : Exp(NodeKind::Identifier), name(n) {}
IdentifierNode::~IdentifierNode() {}

// ===================== IntegerLiteralNode =====================
IntegerLiteralNode::IntegerLiteralNode(long long v)
    : Exp(NodeKind::IntegerLiteral), value(v) {}

// ===================== FloatLiteralNode =====================
FloatLiteralNode::FloatLiteralNode(double v)
    : Exp(NodeKind::FloatLiteral), value(v) {}

// ===================== BoolLiteralNode =====================
BoolLiteralNode::BoolLiteralNode(bool v)
    : Exp(NodeKind::BoolLiteral), value(v) {}

// ===================== CharLiteralNode =====================
CharLiteralNode::CharLiteralNode(char v)
    : Exp(NodeKind::CharLiteral), value(v) {}

// ===================== StringLiteralNode =====================
StringLiteralNode::StringLiteralNode(const string& v)
    : Exp(NodeKind::StringLiteral), value(v) {}

// ===================== ParenthesizedExprNode =====================
ParenthesizedExprNode::ParenthesizedExprNode(Exp* e)
    : Exp(NodeKind::ParenthesizedExpr), expr(e) {}
ParenthesizedExprNode::~ParenthesizedExprNode() { delete expr; }

// ===================== Stm base =====================
Stm::~Stm() {}

// ===================== Body =====================
Body::Body() : Stm(NodeKind::Body) {}
Body::~Body() { for (auto s : stmts) delete s; for (auto v : vdlist) delete v; }

// ===================== ExprStmtNode =====================
ExprStmtNode::ExprStmtNode(Exp* e) : Stm(NodeKind::ExprStmt), expr(e) {}
ExprStmtNode::~ExprStmtNode() { delete expr; }

// ===================== DeclStmt =====================
DeclStmt::DeclStmt(VarDecl* d) : Stm(NodeKind::VariableDecl), decl(d) {}
DeclStmt::~DeclStmt() { delete decl; }

// ===================== IfStmt =====================
IfStmt::IfStmt(Exp* c, Stm* t, Stm* e)
    : Stm(NodeKind::IfStmt), condition(c), then_branch(t), else_branch(e) {}
IfStmt::~IfStmt() { delete condition; delete then_branch; delete else_branch; }

// ===================== WhileStmt =====================
WhileStmt::WhileStmt(Exp* c, Stm* b)
    : Stm(NodeKind::WhileStmt), condition(c), body(b) {}
WhileStmt::~WhileStmt() { delete condition; delete body; }

// ===================== DoWhileStmt =====================
DoWhileStmt::DoWhileStmt(Stm* b, Exp* c)
    : Stm(NodeKind::DoWhileStmt), body(b), condition(c) {}
DoWhileStmt::~DoWhileStmt() { delete body; delete condition; }

// ===================== ForStmt =====================
ForStmt::ForStmt(Stm* i, Exp* c, Exp* inc, Stm* b)
    : Stm(NodeKind::ForStmt), init(i), condition(c), increment(inc), body(b) {}
ForStmt::~ForStmt() { delete init; delete condition; delete increment; delete body; }

// ===================== SwitchStmt =====================
SwitchStmt::SwitchStmt(Exp* e) : Stm(NodeKind::SwitchStmt), expr(e) {}
SwitchStmt::~SwitchStmt() { delete expr; for (auto c : cases) delete c; }

// ===================== CaseClause =====================
CaseClause::CaseClause(Exp* v) : Stm(NodeKind::CaseClause), value(v) {}
CaseClause::~CaseClause() { delete value; for (auto s : body) delete s; }

// ===================== DefaultClause =====================
DefaultClause::DefaultClause() : Stm(NodeKind::DefaultClause) {}
DefaultClause::~DefaultClause() { for (auto s : body) delete s; }

// ===================== BreakStmt =====================
BreakStmt::BreakStmt() : Stm(NodeKind::BreakStmt) {}

// ===================== ContinueStmt =====================
ContinueStmt::ContinueStmt() : Stm(NodeKind::ContinueStmt) {}

// ===================== ReturnStmt =====================
ReturnStmt::ReturnStmt(Exp* e) : Stm(NodeKind::ReturnStmt), expr(e) {}
ReturnStmt::~ReturnStmt() { delete expr; }

// ===================== FreeStmt =====================
FreeStmt::FreeStmt(Exp* e) : Stm(NodeKind::FreeStmt), expr(e) {}
FreeStmt::~FreeStmt() { delete expr; }

// ===================== VarDecl =====================
VarDecl::VarDecl(Exp* t, const string& n)
    : kind(NodeKind::VariableDecl), type(t), name(n), initializer(nullptr) {}
VarDecl::~VarDecl() { delete type; for (auto s : array_sizes) delete s; delete initializer; }

// ===================== FunDecl =====================
FunDecl::FunDecl(Exp* rt, const string& n, Body* b)
    : kind(NodeKind::FunctionDecl), return_type(rt), name(n), body(b), is_template(false) {}
FunDecl::~FunDecl() { delete return_type; for (auto p : params) delete p; delete body; }

// ===================== StructDecl =====================
StructDecl::StructDecl(const string& n)
    : kind(NodeKind::StructDecl), name(n) {}
StructDecl::~StructDecl() { for (auto m : members) delete m; }

// ===================== Program =====================
Program::Program() : kind(NodeKind::Program) {}
Program::~Program() {
    for (auto f : functions) delete f;
    for (auto g : globals) delete g;
    for (auto s : structs) delete s;
    for (auto t : templates) delete t;
}

// ===================== TypeNode =====================
TypeNode* TypeNode::from_basic(const string& name) {
    if (name == "void")   return new PrimitiveTypeNode(PrimitiveTypeNode::VOID);
    if (name == "int")    return new PrimitiveTypeNode(PrimitiveTypeNode::INT);
    if (name == "char")   return new PrimitiveTypeNode(PrimitiveTypeNode::CHAR);
    if (name == "float")  return new PrimitiveTypeNode(PrimitiveTypeNode::FLOAT);
    if (name == "double") return new PrimitiveTypeNode(PrimitiveTypeNode::DOUBLE);
    if (name == "bool")   return new PrimitiveTypeNode(PrimitiveTypeNode::BOOL);
    if (name == "auto")   return new PrimitiveTypeNode(PrimitiveTypeNode::AUTO);
    return new NamedTypeNode(name);
}

// ===================== PrimitiveTypeNode =====================
PrimitiveTypeNode::PrimitiveTypeNode(Prim p)
    : TypeNode(NodeKind::PrimitiveType), prim(p) {}

// ===================== PointerTypeNode =====================
PointerTypeNode::PointerTypeNode(TypeNode* b)
    : TypeNode(NodeKind::PointerType), base(b) {}
PointerTypeNode::~PointerTypeNode() { delete base; }

// ===================== StructTypeNode =====================
StructTypeNode::StructTypeNode(const string& n)
    : TypeNode(NodeKind::StructType), name(n) {}

// ===================== NamedTypeNode =====================
NamedTypeNode::NamedTypeNode(const string& n)
    : TypeNode(NodeKind::NamedType), name(n) {}

// ===================== CaptureNode =====================
CaptureNode::CaptureNode(Mode m, const string& n)
    : Exp(NodeKind::Capture), mode(m), name(n) {}

// ===================== LambdaExprNode =====================
LambdaExprNode::LambdaExprNode(const vector<CaptureNode*>& caps, const vector<VarDecl*>& p, TypeNode* r, Body* b)
    : Exp(NodeKind::LambdaExpr), captures(caps), params(p), return_type(r), body(b) {}
LambdaExprNode::~LambdaExprNode() {
    for (auto c : captures) delete c;
    for (auto p : params) delete p;
    delete return_type;
    delete body;
}

// ===================== TemplateParam =====================
TemplateParam::TemplateParam(const string& n) : kind(NodeKind::TemplateParam), name(n) {}

// ===================== TemplateDecl =====================
TemplateDecl::TemplateDecl(const vector<TemplateParam*>& p, FunDecl* f)
    : kind(NodeKind::TemplateDecl), params(p), func(f), struct_decl(nullptr), is_function(true) {}
TemplateDecl::TemplateDecl(const vector<TemplateParam*>& p, StructDecl* s)
    : kind(NodeKind::TemplateDecl), params(p), func(nullptr), struct_decl(s), is_function(false) {}
TemplateDecl::~TemplateDecl() {
    for (auto p : params) delete p;
    delete func;
    delete struct_decl;
}

// ============================================================
// accept(CodeGenVisitor*) — double dispatch stubs
// ============================================================

void BinaryOpNode::accept(CodeGenVisitor* v) { v->visit(this); }
void UnaryOpNode::accept(CodeGenVisitor* v) { v->visit(this); }
void AssignmentNode::accept(CodeGenVisitor* v) { v->visit(this); }
void TernaryOpNode::accept(CodeGenVisitor* v) { v->visit(this); }
void FcallNode::accept(CodeGenVisitor* v) { v->visit(this); }
void IndexNode::accept(CodeGenVisitor* v) { v->visit(this); }
void MemberAccessNode::accept(CodeGenVisitor* v) { v->visit(this); }
void ArrowAccessNode::accept(CodeGenVisitor* v) { v->visit(this); }
void MallocNode::accept(CodeGenVisitor* v) { v->visit(this); }
void CastNode::accept(CodeGenVisitor* v) { v->visit(this); }
void SizeOfNode::accept(CodeGenVisitor* v) { v->visit(this); }
void IdentifierNode::accept(CodeGenVisitor* v) { v->visit(this); }
void IntegerLiteralNode::accept(CodeGenVisitor* v) { v->visit(this); }
void FloatLiteralNode::accept(CodeGenVisitor* v) { v->visit(this); }
void BoolLiteralNode::accept(CodeGenVisitor* v) { v->visit(this); }
void CharLiteralNode::accept(CodeGenVisitor* v) { v->visit(this); }
void StringLiteralNode::accept(CodeGenVisitor* v) { v->visit(this); }
void ParenthesizedExprNode::accept(CodeGenVisitor* v) { v->visit(this); }
void PrimitiveTypeNode::accept(CodeGenVisitor* v) { v->visit(this); }
void PointerTypeNode::accept(CodeGenVisitor* v) { v->visit(this); }
void StructTypeNode::accept(CodeGenVisitor* v) { v->visit(this); }
void NamedTypeNode::accept(CodeGenVisitor* v) { v->visit(this); }
void CaptureNode::accept(CodeGenVisitor* v) { v->visit(this); }
void LambdaExprNode::accept(CodeGenVisitor* v) { v->visit(this); }

void Body::accept(CodeGenVisitor* v) { v->visit(this); }
void ExprStmtNode::accept(CodeGenVisitor* v) { v->visit(this); }
void DeclStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void IfStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void WhileStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void DoWhileStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void ForStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void SwitchStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void CaseClause::accept(CodeGenVisitor* v) { v->visit(this); }
void DefaultClause::accept(CodeGenVisitor* v) { v->visit(this); }
void BreakStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void ContinueStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void ReturnStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void FreeStmt::accept(CodeGenVisitor* v) { v->visit(this); }

void VarDecl::accept(CodeGenVisitor* v) { v->visit(this); }
void FunDecl::accept(CodeGenVisitor* v) { v->visit(this); }
void StructDecl::accept(CodeGenVisitor* v) { v->visit(this); }
void Program::accept(CodeGenVisitor* v) { v->visit(this); }
void TemplateDecl::accept(CodeGenVisitor* v) { v->visit(this); }

// ============================================================
// computeAddress(CodeGenVisitor*) — lvalue nodes only
// ============================================================

void UnaryOpNode::computeAddress(CodeGenVisitor* v) { v->computeAddress(this); }
void IdentifierNode::computeAddress(CodeGenVisitor* v) { v->computeAddress(this); }
void IndexNode::computeAddress(CodeGenVisitor* v) { v->computeAddress(this); }
void MemberAccessNode::computeAddress(CodeGenVisitor* v) { v->computeAddress(this); }
void ArrowAccessNode::computeAddress(CodeGenVisitor* v) { v->computeAddress(this); }
