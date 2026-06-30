#include "ast.h"
#include <iostream>

using namespace std;

// ===================== Exp base =====================

double Exp::accept(Visitor*) {
    return 0.0;
}

Type* Exp::accept(TypeVisitor*) {
    return nullptr;
}

void Exp::accept(CodeGenVisitor*) {
}

void Exp::computeAddress(CodeGenVisitor*) {
    cerr << "Error: el nodo no es un lvalue" << endl;
}

// ===================== BinaryOpNode =====================
BinaryOpNode::BinaryOpNode(Exp* l, Exp* r, BinaryOp o)
    : Exp(), left(l), right(r), op(o) {}
BinaryOpNode::~BinaryOpNode() { delete left; delete right; }

// ===================== UnaryOpNode =====================
UnaryOpNode::UnaryOpNode(Exp* o, UnaryOp uop)
    : Exp(), operand(o), op(uop) {}
UnaryOpNode::~UnaryOpNode() { delete operand; }

// ===================== AssignmentNode =====================
AssignmentNode::AssignmentNode(Exp* t, Exp* v, AssignOp o)
    : Exp(), target(t), value(v), op(o) {}
AssignmentNode::~AssignmentNode() { delete target; delete value; }



// ===================== FcallNode =====================
FcallNode::FcallNode(Exp* c) : Exp(), callee(c) {}
FcallNode::~FcallNode() { delete callee; for (auto a : args) delete a; }

// ===================== MallocNode =====================
MallocNode::MallocNode(Exp* s) : Exp(), size(s) {}
MallocNode::~MallocNode() { delete size; }

// ===================== IndexNode =====================
IndexNode::IndexNode(Exp* b, Exp* i)
    : Exp(), base(b), index(i) {}
IndexNode::~IndexNode() { delete base; delete index; }

// ===================== MemberAccessNode =====================
MemberAccessNode::MemberAccessNode(Exp* o, const string& m)
    : Exp(), object(o), member(m) {}
MemberAccessNode::~MemberAccessNode() { delete object; }

// ===================== ArrowAccessNode =====================
ArrowAccessNode::ArrowAccessNode(Exp* p, const string& m)
    : Exp(), pointer(p), member(m) {}
ArrowAccessNode::~ArrowAccessNode() { delete pointer; }



// ===================== SizeOfNode =====================
SizeOfNode::SizeOfNode(Exp* t) : Exp(), target_type(t) {}
SizeOfNode::~SizeOfNode() { delete target_type; }

// ===================== IdentifierNode =====================
IdentifierNode::IdentifierNode(const string& n)
    : Exp(), name(n) {}
IdentifierNode::~IdentifierNode() {}

// ===================== IntegerLiteralNode =====================
IntegerLiteralNode::IntegerLiteralNode(long long v)
    : Exp(), value(v) {}

// ===================== FloatLiteralNode =====================
FloatLiteralNode::FloatLiteralNode(double v)
    : Exp(), value(v) {}

// ===================== BoolLiteralNode =====================
BoolLiteralNode::BoolLiteralNode(bool v)
    : Exp(), value(v) {}

// ===================== CharLiteralNode =====================
CharLiteralNode::CharLiteralNode(char v)
    : Exp(), value(v) {}

// ===================== StringLiteralNode =====================
StringLiteralNode::StringLiteralNode(const string& v)
    : Exp(), value(v) {}


// ===================== PrintfNode =====================
PrintfNode::PrintfNode() : Exp(), format("%ld") {}
PrintfNode::PrintfNode(const string& fmt, const vector<Exp*>& a) : Exp(), format(fmt), args(a) {}
PrintfNode::~PrintfNode() { for (auto a : args) delete a; }

// ===================== Stm base =====================
Stm::~Stm() {}

// ===================== Body =====================
Body::Body() : Stm() {}
Body::~Body() { for (auto s : stmts) delete s; }

// ===================== ExprStmtNode =====================
ExprStmtNode::ExprStmtNode(Exp* e) : Stm(), expr(e) {}
ExprStmtNode::~ExprStmtNode() { delete expr; }

// ===================== IfStmt =====================
IfStmt::IfStmt(Exp* c, Stm* t, Stm* e)
    : Stm(), condition(c), then_branch(t), else_branch(e) {}
IfStmt::~IfStmt() { delete condition; delete then_branch; delete else_branch; }

// ===================== WhileStmt =====================
WhileStmt::WhileStmt(Exp* c, Stm* b)
    : Stm(), condition(c), body(b) {}
WhileStmt::~WhileStmt() { delete condition; delete body; }

// ===================== DoWhileStmt =====================
DoWhileStmt::DoWhileStmt(Stm* b, Exp* c)
    : Stm(), body(b), condition(c) {}
DoWhileStmt::~DoWhileStmt() { delete body; delete condition; }

// ===================== ForStmt =====================
ForStmt::ForStmt(Stm* i, Exp* c, Exp* inc, Stm* b)
    : Stm(), init(i), condition(c), increment(inc), body(b) {}
ForStmt::~ForStmt() { delete init; delete condition; delete increment; delete body; }

// ===================== SwitchStmt =====================
SwitchStmt::SwitchStmt(Exp* e) : Stm(), expr(e) {}
SwitchStmt::~SwitchStmt() { delete expr; for (auto c : cases) delete c; for (auto s : default_body) delete s; }

// ===================== CaseClause =====================
CaseClause::CaseClause(Exp* v) : Stm(), value(v) {}
CaseClause::~CaseClause() { delete value; for (auto s : body) delete s; }

// ===================== BreakStmt =====================
BreakStmt::BreakStmt() : Stm() {}

// ===================== ContinueStmt =====================
ContinueStmt::ContinueStmt() : Stm() {}

// ===================== ReturnStmt =====================
ReturnStmt::ReturnStmt(Exp* e) : Stm(), expr(e) {}
ReturnStmt::~ReturnStmt() { delete expr; }

// ===================== FreeStmt =====================
FreeStmt::FreeStmt(Exp* e) : Stm(), expr(e) {}
FreeStmt::~FreeStmt() { delete expr; }

// ===================== VarDecl =====================
VarDecl::VarDecl(Exp* t, const string& n)
    : Stm(), type(t), name(n), initializer(nullptr) {}
VarDecl::~VarDecl() { delete type; for (auto s : array_sizes) delete s; delete initializer; }

// ===================== FunDecl =====================
FunDecl::FunDecl(Exp* rt, const string& n, Body* b)
    : return_type(rt), name(n), body(b), is_template(false) {}
FunDecl::~FunDecl() { delete return_type; for (auto p : params) delete p; delete body; }

// ===================== StructDecl =====================
StructDecl::StructDecl(const string& n)
    : name(n) {}
StructDecl::~StructDecl() { for (auto m : members) delete m; }

// ===================== Program =====================
Program::Program() {}
Program::~Program() {
    for (auto f : functions) delete f;
    for (auto g : globals) delete g;
    for (auto s : structs) delete s;
    for (auto t : templates) delete t;
}

// ===================== PrimitiveTypeNode =====================
PrimitiveTypeNode::PrimitiveTypeNode(Prim p)
    : TypeNode(), prim(p) {}

// ===================== PointerTypeNode =====================
PointerTypeNode::PointerTypeNode(TypeNode* b)
    : TypeNode(), base(b) {}
PointerTypeNode::~PointerTypeNode() { delete base; }

// ===================== StructTypeNode =====================
StructTypeNode::StructTypeNode(const string& n)
    : TypeNode(), name(n) {}

// ===================== NamedTypeNode =====================
NamedTypeNode::NamedTypeNode(const string& n)
    : TypeNode(), name(n) {}

// ===================== TemplateTypeNode =====================
TemplateTypeNode::TemplateTypeNode(const string& n, const vector<TypeNode*>& args)
    : TypeNode(), name(n), type_args(args) {}
TemplateTypeNode::~TemplateTypeNode() {
    for (auto a : type_args) delete a;
}

// ===================== CaptureNode =====================
CaptureNode::CaptureNode(Mode m, const string& n)
    : Exp(), mode(m), name(n) {}

// ===================== LambdaExprNode =====================
LambdaExprNode::LambdaExprNode(const vector<CaptureNode*>& caps, const vector<VarDecl*>& p, TypeNode* r, Body* b)
    : Exp(), captures(caps), params(p), return_type(r), body(b) {}
LambdaExprNode::~LambdaExprNode() {
    for (auto c : captures) delete c;
    for (auto p : params) delete p;
    delete return_type;
    delete body;
}

// ===================== TemplateDecl =====================
TemplateDecl::TemplateDecl(const vector<string>& p, FunDecl* f)
    : params(p), func(f), struct_decl(nullptr), is_function(true) {}
TemplateDecl::TemplateDecl(const vector<string>& p, StructDecl* s)
    : params(p), func(nullptr), struct_decl(s), is_function(false) {}
TemplateDecl::~TemplateDecl() {
    delete func;
    delete struct_decl;
}

