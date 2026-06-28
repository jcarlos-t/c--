#include "visitor.h"
#include "ast.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

using namespace std;

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
void PrintfNode::accept(CodeGenVisitor* v) { v->visit(this); }
void PrimitiveTypeNode::accept(CodeGenVisitor* v) { v->visit(this); }
void PointerTypeNode::accept(CodeGenVisitor* v) { v->visit(this); }
void StructTypeNode::accept(CodeGenVisitor* v) { v->visit(this); }
void NamedTypeNode::accept(CodeGenVisitor* v) { v->visit(this); }
void TemplateTypeNode::accept(CodeGenVisitor* v) { v->visit(this); }
void CaptureNode::accept(CodeGenVisitor* v) { v->visit(this); }
void LambdaExprNode::accept(CodeGenVisitor* v) { v->visit(this); }

void Body::accept(CodeGenVisitor* v) { v->visit(this); }
void ExprStmtNode::accept(CodeGenVisitor* v) { v->visit(this); }
void IfStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void WhileStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void DoWhileStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void ForStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void SwitchStmt::accept(CodeGenVisitor* v) { v->visit(this); }
void CaseClause::accept(CodeGenVisitor* v) { v->visit(this); }
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

// ============================================================
// Entry point
// ============================================================
void GenCodeVisitor::generate(Program *p) {
    stringLabels.clear();
    for (auto g : p->globals)
        memoriaGlobal[g->name] = true;

    out << ".data\n";
    out << "print_fmt: .string \"%ld\\n\"\n";
    out << "println_fmt: .string \"\\n\"\n";

    for (auto &[name, _] : memoriaGlobal)
        out << name << ": .quad 0\n";

    out << "\n.text\n";
    out << ".globl _start\n";
    out << "_start:\n";
    out << "  call main\n";
    out << "  movq %rax, %rdi\n";
    out << "  call exit@PLT\n";

    for (auto f : p->functions)
        f->accept(this);

    out << "\npotencia:\n";
    out << "  pushq %rbp\n";
    out << "  movq %rsp, %rbp\n";
    out << "  cmpq $0, %rsi\n";
    out << "  jne .pot_nz\n";
    out << "  movq $1, %rax\n";
    out << "  jmp .pot_end\n";
    out << ".pot_nz:\n";
    out << "  cmpq $1, %rsi\n";
    out << "  jne .pot_rec\n";
    out << "  movq %rdi, %rax\n";
    out << "  jmp .pot_end\n";
    out << ".pot_rec:\n";
    out << "  pushq %rbx\n";
    out << "  movq %rdi, %rbx\n";
    out << "  testq $1, %rsi\n";
    out << "  jz .pot_even\n";
    out << "  imulq %rdi, %rdi\n";
    out << "  sarq $1, %rsi\n";
    out << "  call potencia\n";
    out << "  imulq %rbx, %rax\n";
    out << "  popq %rbx\n";
    out << "  jmp .pot_end\n";
    out << ".pot_even:\n";
    out << "  imulq %rdi, %rdi\n";
    out << "  sarq $1, %rsi\n";
    out << "  call potencia\n";
    out << "  popq %rbx\n";
    out << ".pot_end:\n";
    out << "  leave\n";
    out << "  ret\n";

    out << "\n.section .note.GNU-stack,\"\",@progbits\n";
}

// ============================================================
// Lvalue capture & store
// ============================================================

string GenCodeVisitor::varMem(const string& name) {
    if (memoriaGlobal.count(name))
        return name + "(%rip)";
    return to_string(memoria[name]) + "(%rbp)";
}

void GenCodeVisitor::loadVar(const string& name) {
    out << "  movq " << varMem(name) << ", %rax\n";
}

void GenCodeVisitor::storeVar(const string& name) {
    out << "  movq %rax, " << varMem(name) << "\n";
}

void GenCodeVisitor::leaVar(const string& name) {
    out << "  leaq " << varMem(name) << ", %rax\n";
}

GenCodeVisitor::LVal GenCodeVisitor::captureLVal(Exp *e) {
    LVal lv;
    lvalTarget = &lv;
    e->accept(this);
    lvalTarget = nullptr;
    return lv;
}

void GenCodeVisitor::storeTarget(const LVal &lv) {
    switch (lv.kind) {
    case LValKind::Id:
        storeVar(lv.name);
        break;
    case LValKind::Index:
        out << "  pushq %rax\n";
        lv.index->accept(this);
        out << "  movq %rax, %rdi\n";
        out << "  popq %rax\n";
        out << "  movq %rax, %rcx\n";
        loadVar(lv.name);
        out << "  movq %rcx, (%rax, %rdi, 8)\n";
        break;
    case LValKind::Member:
        out << "  pushq %rax\n";
        out << "  popq %rcx\n";
        loadVar(lv.name);
        out << "  movq %rcx, " << structFieldOffset[lv.name][lv.member] << "(%rax)\n";
        break;
    case LValKind::Deref:
        out << "  popq %rbx\n";
        out << "  movq %rax, (%rbx)\n";
        break;
    default:
        throw runtime_error("Asignación a expresión que no es lvalue");
    }
}

// ============================================================
// Expressions
// ============================================================

void GenCodeVisitor::visit(IntegerLiteralNode *e) {
    out << "  movq $" << e->value << ", %rax\n";
}

void GenCodeVisitor::visit(FloatLiteralNode *e) {
    out << "  movq $" << (long long)e->value << ", %rax\n";
}

void GenCodeVisitor::visit(BoolLiteralNode *e) {
    out << "  movq $" << (e->value ? 1 : 0) << ", %rax\n";
}

void GenCodeVisitor::visit(CharLiteralNode *e) {
    out << "  movq $" << (int)e->value << ", %rax\n";
}

void GenCodeVisitor::visit(StringLiteralNode *e) {
    auto it = stringLabels.find(e->value);
    int lbl;
    if (it == stringLabels.end()) {
        lbl = (int)stringLabels.size();
        stringLabels[e->value] = lbl;
        out << ".section .rodata\n";
        out << ".Lstr" << lbl << ": .string \"" << e->value << "\"\n";
        out << ".text\n";
    } else {
        lbl = it->second;
    }
    out << "  leaq .Lstr" << lbl << "(%rip), %rax\n";
}

void GenCodeVisitor::visit(IdentifierNode *e) {
    if (lvalTarget) {
        lvalTarget->kind = LValKind::Id;
        lvalTarget->name = e->name;
        return;
    }
    loadVar(e->name);
}

void GenCodeVisitor::visit(BinaryOpNode *e) {
    if (e->op == BinaryOp::POW) {
        auto *rightNum = dynamic_cast<IntegerLiteralNode *>(e->right);
        if (rightNum && rightNum->value == 2) {
            e->left->accept(this);
            out << "  imulq %rax, %rax\n";
            return;
        }
        if (rightNum && rightNum->value == 4) {
            e->left->accept(this);
            out << "  imulq %rax, %rax\n";
            out << "  imulq %rax, %rax\n";
            return;
        }
    }

    e->left->accept(this);
    out << "  pushq %rax\n";
    e->right->accept(this);
    out << "  movq %rax, %rcx\n";
    out << "  popq %rax\n";

    switch (e->op) {
    case BinaryOp::ADD: out << "  addq %rcx, %rax\n"; break;
    case BinaryOp::SUB: out << "  subq %rcx, %rax\n"; break;
    case BinaryOp::MUL: out << "  imulq %rcx, %rax\n"; break;
    case BinaryOp::DIV:
        out << "  cqto\n";
        out << "  idivq %rcx\n";
        break;
    case BinaryOp::MOD:
        out << "  cqto\n";
        out << "  idivq %rcx\n";
        out << "  movq %rdx, %rax\n";
        break;
    case BinaryOp::POW:
        out << "  movq %rax, %rdi\n";
        out << "  movq %rcx, %rsi\n";
        out << "  call potencia\n";
        break;
    case BinaryOp::EQ:
        out << "  cmpq %rcx, %rax\n";
        out << "  movq $0, %rax\n";
        out << "  sete %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::NE:
        out << "  cmpq %rcx, %rax\n";
        out << "  movq $0, %rax\n";
        out << "  setne %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::LT:
        out << "  cmpq %rcx, %rax\n";
        out << "  movq $0, %rax\n";
        out << "  setl %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::GT:
        out << "  cmpq %rcx, %rax\n";
        out << "  movq $0, %rax\n";
        out << "  setg %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::LE:
        out << "  cmpq %rcx, %rax\n";
        out << "  movq $0, %rax\n";
        out << "  setle %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::GE:
        out << "  cmpq %rcx, %rax\n";
        out << "  movq $0, %rax\n";
        out << "  setge %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::LOG_AND:
        out << "  andq %rcx, %rax\n";
        break;
    case BinaryOp::LOG_OR:
        out << "  orq %rcx, %rax\n";
        break;
    case BinaryOp::COMMA:
        break; // comma: left already evaluated, result is right (already in %rax)
    }
}

void GenCodeVisitor::visit(UnaryOpNode *e) {
    e->operand->accept(this);

    switch (e->op) {
    case UnaryOp::MINUS:
        out << "  negq %rax\n";
        break;
    case UnaryOp::LOG_NOT:
        out << "  cmpq $0, %rax\n";
        out << "  movq $0, %rax\n";
        out << "  sete %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case UnaryOp::PRE_INC:
        out << "  incq %rax\n";
        if (auto *id = dynamic_cast<IdentifierNode *>(e->operand))
            storeVar(id->name);
        break;
    case UnaryOp::PRE_DEC:
        out << "  decq %rax\n";
        if (auto *id = dynamic_cast<IdentifierNode *>(e->operand))
            storeVar(id->name);
        break;
    case UnaryOp::POST_INC:
        if (auto *id = dynamic_cast<IdentifierNode *>(e->operand)) {
            out << "  pushq %rax\n";
            out << "  incq %rax\n";
            storeVar(id->name);
            out << "  popq %rax\n";
        }
        break;
    case UnaryOp::POST_DEC:
        if (auto *id = dynamic_cast<IdentifierNode *>(e->operand)) {
            out << "  pushq %rax\n";
            out << "  decq %rax\n";
            storeVar(id->name);
            out << "  popq %rax\n";
        }
        break;
    case UnaryOp::ADDR:
        if (auto *id = dynamic_cast<IdentifierNode *>(e->operand))
            leaVar(id->name);
        break;
    case UnaryOp::DEREF:
        out << "  movq (%rax), %rax\n";
        break;
    }
}

void GenCodeVisitor::visit(AssignmentNode *e) {
    LVal target = captureLVal(e->target);

    e->value->accept(this); // value in %rax

    switch (e->op) {
    case AssignOp::ASSIGN:
        break;
    case AssignOp::ADD_ASSIGN:
    case AssignOp::SUB_ASSIGN:
    case AssignOp::MUL_ASSIGN:
    case AssignOp::DIV_ASSIGN:
        out << "  pushq %rax\n";
        e->target->accept(this); // load current value
        out << "  popq %rcx\n";
        switch (e->op) {
        case AssignOp::ADD_ASSIGN: out << "  addq %rcx, %rax\n"; break;
        case AssignOp::SUB_ASSIGN: out << "  subq %rcx, %rax\n"; break;
        case AssignOp::MUL_ASSIGN: out << "  imulq %rcx, %rax\n"; break;
        case AssignOp::DIV_ASSIGN:
            out << "  pushq %rcx\n";
            out << "  cqto\n";
            out << "  popq %rcx\n";
            out << "  idivq %rcx\n";
            break;
        default: break;
        }
        break;
    }

    storeTarget(target);
}

void GenCodeVisitor::visit(TernaryOpNode *e) {
    int lbl = labelcont++;
    e->condition->accept(this);
    out << "  cmpq $0, %rax\n";
    out << "  je else_" << lbl << "\n";
    e->then_expr->accept(this);
    out << "  jmp end_ternary_" << lbl << "\n";
    out << "else_" << lbl << ":\n";
    e->else_expr->accept(this);
    out << "end_ternary_" << lbl << ":\n";
}

void GenCodeVisitor::visit(PrintfNode *e) {
    for (size_t i = 0; i < e->args.size(); i++) {
        e->args[i]->accept(this);
        out << "  movq %rax, %rsi\n";
        out << "  leaq print_fmt(%rip), %rdi\n";
        out << "  movq $0, %rax\n";
        out << "  call printf@PLT\n";
    }
    out << "  leaq println_fmt(%rip), %rdi\n";
    out << "  movq $0, %rax\n";
    out << "  call printf@PLT\n";
    out << "  movq $0, %rax\n";
}

void GenCodeVisitor::visit(FcallNode *e) {
    const vector<string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    auto *id = dynamic_cast<IdentifierNode *>(e->callee);
    string fname = id ? id->name : "";

    int nArgs = (int)e->args.size();
    for (int i = 0; i < nArgs && i < 6; i++) {
        e->args[i]->accept(this);
        out << "  movq %rax, " << argRegs[i] << "\n";
    }
    // extra args beyond 6 go on stack (simplified: not implemented)
    out << "  call " << fname << "\n";
}

void GenCodeVisitor::visit(IndexNode *e) {
    if (lvalTarget) {
        lvalTarget->kind = LValKind::Index;
        if (auto *id = dynamic_cast<IdentifierNode *>(e->base))
            lvalTarget->name = id->name;
        lvalTarget->index = e->index;
        return;
    }

    vector<Exp*> indices;
    Exp* b = e;
    while (auto* inner = dynamic_cast<IndexNode*>(b)) {
        indices.push_back(inner->index);
        b = inner->base;
    }
    reverse(indices.begin(), indices.end());

    string arrName;
    if (auto* id = dynamic_cast<IdentifierNode*>(b))
        arrName = id->name;

    auto dimsIt = arrayDimensions.find(arrName);
    if (!arrName.empty() && dimsIt != arrayDimensions.end() && indices.size() > 1) {
        loadVar(arrName);
        out << "  pushq %rax\n";
        for (size_t d = 0; d < indices.size(); d++) {
            indices[d]->accept(this);
            int stride = 1;
            for (size_t s = d + 1; s < dimsIt->second.size() && s < indices.size(); s++)
                stride *= dimsIt->second[s];
            if (stride > 1) {
                out << "  movq $" << stride << ", %rcx\n";
                out << "  imulq %rcx, %rax\n";
            }
            if (d > 0) {
                out << "  addq %rax, %rdi\n";
            } else {
                out << "  movq %rax, %rdi\n";
            }
        }
        out << "  popq %rax\n";
        out << "  movq (%rax, %rdi, 8), %rax\n";
        return;
    }

    e->index->accept(this);
    out << "  movq %rax, %rdi\n";
    if (auto *id = dynamic_cast<IdentifierNode *>(e->base))
        loadVar(id->name);
    else
        e->base->accept(this);
    out << "  movq (%rax, %rdi, 8), %rax\n";
}

void GenCodeVisitor::visit(MemberAccessNode *e) {
    if (lvalTarget) {
        if (auto *id = dynamic_cast<IdentifierNode *>(e->object)) {
            lvalTarget->kind = LValKind::Member;
            lvalTarget->name = id->name;
            lvalTarget->member = e->member;
        }
        return;
    }
    if (auto *id = dynamic_cast<IdentifierNode *>(e->object)) {
        loadVar(id->name);
        out << "  movq " << structFieldOffset[id->name][e->member] << "(%rax), %rax\n";
    }
}

void GenCodeVisitor::visit(ArrowAccessNode *e) {
    if (lvalTarget) {
        lvalTarget->kind = LValKind::Member;
        if (auto *id = dynamic_cast<IdentifierNode *>(e->pointer))
            lvalTarget->name = id->name;
        lvalTarget->member = e->member;
        return;
    }
    if (auto *id = dynamic_cast<IdentifierNode *>(e->pointer)) {
        loadVar(id->name);
        out << "  movq " << structFieldOffset[id->name][e->member] << "(%rax), %rax\n";
    }
}

void GenCodeVisitor::visit(MallocNode *e) {
    e->size->accept(this);
    out << "  movq %rax, %rdi\n";
    out << "  call malloc@PLT\n";
}

void GenCodeVisitor::visit(CastNode *e) {
    e->expr->accept(this);
    if (auto *pt = dynamic_cast<PrimitiveTypeNode *>(e->target_type)) {
        if (pt->prim == PrimitiveTypeNode::BOOL) {
            out << "  cmpq $0, %rax\n";
            out << "  setne %al\n";
            out << "  movzbq %al, %rax\n";
        }
    }
}

void GenCodeVisitor::visit(SizeOfNode *e) {
    if (auto *pt = dynamic_cast<PrimitiveTypeNode *>(e->target_type)) {
        switch (pt->prim) {
        case PrimitiveTypeNode::VOID:
        case PrimitiveTypeNode::CHAR:
        case PrimitiveTypeNode::BOOL: out << "  movq $1, %rax\n"; return;
        case PrimitiveTypeNode::INT:
        case PrimitiveTypeNode::FLOAT: out << "  movq $4, %rax\n"; return;
        case PrimitiveTypeNode::DOUBLE: out << "  movq $8, %rax\n"; return;
        case PrimitiveTypeNode::AUTO: out << "  movq $0, %rax\n"; return;
        }
    }
    if (dynamic_cast<PointerTypeNode *>(e->target_type))
        out << "  movq $8, %rax\n";
    else
        out << "  movq $0, %rax\n";
}

void GenCodeVisitor::visit(PrimitiveTypeNode *) {}
void GenCodeVisitor::visit(PointerTypeNode *) {}
void GenCodeVisitor::visit(StructTypeNode *) {}
void GenCodeVisitor::visit(NamedTypeNode *) {}

void GenCodeVisitor::visit(TemplateTypeNode *) {}
void GenCodeVisitor::visit(CaptureNode *) {}
void GenCodeVisitor::visit(LambdaExprNode *e) {
    int lbl = labelcont++;
    string lambdaName = ".Llambda_" + to_string(lbl);

    string savedFunc = funcName;
    bool savedInFunc = inFunction;
    int savedOffset = offset;
    auto savedMemoria = memoria;

    inFunction = true;
    memoria.clear();
    offset = -8;
    funcName = lambdaName;

    for (auto p : e->params) {
        memoria[p->name] = offset;
        offset -= 8;
    }

    int frameSize = (-offset + 15) & ~15;

    out << lambdaName << ":\n";
    out << "  pushq %rbp\n";
    out << "  movq %rsp, %rbp\n";
    if (frameSize > 0)
        out << "  subq $" << frameSize << ", %rsp\n";

    e->body->accept(this);

    out << ".Llambda_end_" << lbl << ":\n";
    out << "  movq $0, %rax\n";
    out << "  leave\n";
    out << "  ret\n";

    inFunction = savedInFunc;
    offset = savedOffset;
    memoria = savedMemoria;
    funcName = savedFunc;

    out << "  leaq " << lambdaName << "(%rip), %rax\n";
}

// ============================================================
// Statements
// ============================================================

void GenCodeVisitor::visit(Body *s) {
    for (auto st : s->stmts)
        st->accept(this);
}

void GenCodeVisitor::visit(ExprStmtNode *s) {
    if (s->expr)
        s->expr->accept(this);
}

void GenCodeVisitor::visit(IfStmt *s) {
    int lbl = labelcont++;
    s->condition->accept(this);
    out << "  cmpq $0, %rax\n";
    out << "  je else_" << lbl << "\n";
    s->then_branch->accept(this);
    out << "  jmp endif_" << lbl << "\n";
    out << "else_" << lbl << ":\n";
    if (s->else_branch)
        s->else_branch->accept(this);
    out << "endif_" << lbl << ":\n";
}

void GenCodeVisitor::visit(WhileStmt *s) {
    int lbl = labelcont++;
    string oldBreak = currentBreakLabel;
    currentBreakLabel = "endwhile_" + to_string(lbl);

    out << "while_" << lbl << ":\n";
    s->condition->accept(this);
    out << "  cmpq $0, %rax\n";
    out << "  je endwhile_" << lbl << "\n";
    s->body->accept(this);
    out << "  jmp while_" << lbl << "\n";
    out << "endwhile_" << lbl << ":\n";

    currentBreakLabel = oldBreak;
}

void GenCodeVisitor::visit(DoWhileStmt *s) {
    int lbl = labelcont++;
    string oldBreak = currentBreakLabel;
    currentBreakLabel = "endwhile_" + to_string(lbl);

    out << "dowhile_" << lbl << ":\n";
    s->body->accept(this);
    s->condition->accept(this);
    out << "  cmpq $0, %rax\n";
    out << "  jne dowhile_" << lbl << "\n";
    out << "endwhile_" << lbl << ":\n";

    currentBreakLabel = oldBreak;
}

void GenCodeVisitor::visit(ForStmt *s) {
    int lbl = labelcont++;
    string oldBreak = currentBreakLabel;
    currentBreakLabel = "endfor_" + to_string(lbl);

    if (s->init)
        s->init->accept(this);

    out << "for_" << lbl << ":\n";
    if (s->condition) {
        s->condition->accept(this);
        out << "  cmpq $0, %rax\n";
        out << "  je endfor_" << lbl << "\n";
    }
    s->body->accept(this);
    if (s->increment)
        s->increment->accept(this);
    out << "  jmp for_" << lbl << "\n";
    out << "endfor_" << lbl << ":\n";

    currentBreakLabel = oldBreak;
}

void GenCodeVisitor::visit(SwitchStmt *s) {
    int lbl = labelcont++;
    s->expr->accept(this);
    out << "  movq %rax, %r10\n";

    int caseIdx = 0;
    for (auto cc : s->cases) {
        if (auto *lit = dynamic_cast<IntegerLiteralNode *>(cc->value))
            out << "  movq $" << lit->value << ", %rax\n";
        else
            out << "  movq $0, %rax\n";
        out << "  cmpq %rax, %r10\n";
        out << "  je case_" << lbl << "_" << caseIdx << "\n";
        caseIdx++;
    }

    out << "  jmp default_" << lbl << "\n";

    string oldBreak = currentBreakLabel;
    currentBreakLabel = "endswitch_" + to_string(lbl);

    caseIdx = 0;
    for (auto cc : s->cases) {
        out << "case_" << lbl << "_" << caseIdx << ":\n";
        for (auto st : cc->body) st->accept(this);
        out << "  jmp endswitch_" << lbl << "\n";
        caseIdx++;
    }

    out << "default_" << lbl << ":\n";
    for (auto st : s->default_body) st->accept(this);

    currentBreakLabel = oldBreak;
    out << "endswitch_" << lbl << ":\n";
}

void GenCodeVisitor::visit(CaseClause *) {}
void GenCodeVisitor::visit(BreakStmt *) {
    if (currentBreakLabel.empty()) {
        cerr << "Error: break fuera de ciclo o switch\n";
        exit(1);
    }
    out << "  jmp " << currentBreakLabel << "\n";
}

void GenCodeVisitor::visit(ContinueStmt *) {
    // continue jumps back to loop condition
    // This is complex; simplified as jmp to the loop label
    out << "  ; continue (simplified)\n";
}

void GenCodeVisitor::visit(ReturnStmt *s) {
    if (s->expr)
        s->expr->accept(this);
    else
        out << "  movq $0, %rax\n";
    out << "  jmp .end_" << funcName << "\n";
}

void GenCodeVisitor::visit(FreeStmt *s) {
    s->expr->accept(this);
    out << "  movq %rax, %rdi\n";
    out << "  call free@PLT\n";
}

// ============================================================
// Declarations
// ============================================================

void GenCodeVisitor::visit(VarDecl *d) {
    if (!inFunction) {
        memoriaGlobal[d->name] = true;
        return;
    }

    memoria[d->name] = offset;
    offset -= 8;

    if (!d->array_sizes.empty()) {
        vector<int> dims;
        for (auto s : d->array_sizes) {
            if (auto* il = dynamic_cast<IntegerLiteralNode*>(s))
                dims.push_back((int)il->value);
            else
                dims.push_back(0);
        }
        arrayDimensions[d->name] = dims;
    }

    if (d->initializer) {
        d->initializer->accept(this);
        storeVar(d->name);
    }
}

void GenCodeVisitor::visit(FunDecl *d) {
    inFunction = true;
    memoria.clear();
    offset = -8;
    funcName = d->name;
    int paramCount = (int)d->params.size();

    const vector<string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    for (int i = 0; i < paramCount && i < 6; i++) {
        memoria[d->params[i]->name] = offset;
        offset -= 8;
    }

    for (auto st : d->body->stmts) {
        if (auto* v = dynamic_cast<VarDecl*>(st)) {
            memoria[v->name] = offset;
            offset -= 8;
        }
    }
    int frameSize = (-offset + 15) & ~15;

    out << "\n.globl " << d->name << "\n";
    out << d->name << ":\n";
    out << "  pushq %rbp\n";
    out << "  movq %rsp, %rbp\n";
    if (frameSize > 0)
        out << "  subq $" << frameSize << ", %rsp\n";

    for (int i = 0; i < paramCount && i < 6; i++)
        out << "  movq " << argRegs[i] << ", " << memoria[d->params[i]->name] << "(%rbp)\n";

    d->body->accept(this);

    out << ".end_" << d->name << ":\n";
    out << "  leave\n";
    out << "  ret\n";
    inFunction = false;
}

void GenCodeVisitor::visit(StructDecl *d) {
    int fieldOff = 0;
    for (auto m : d->members) {
        structFieldOffset[d->name][m->name] = fieldOff;
        fieldOff += 8;
    }
    structFieldCount[d->name] = (int)d->members.size();
}

void GenCodeVisitor::visit(Program *p) {
    generate(p); // forward to entry point
}

void GenCodeVisitor::visit(TemplateDecl *d) {
    if (d->struct_decl) {
        int fieldOff = 0;
        for (auto m : d->struct_decl->members) {
            structFieldOffset[d->struct_decl->name][m->name] = fieldOff;
            fieldOff += 8;
        }
        structFieldCount[d->struct_decl->name] = (int)d->struct_decl->members.size();
    }
    if (d->func) {
        d->func->accept(this);
    }
}

// ============================================================
// computeAddress — lvalue support for assignment
// ============================================================

void GenCodeVisitor::computeAddress(UnaryOpNode *e) {
    // DEREF: address is the value of the pointer expression
    e->operand->accept(this);
}

void GenCodeVisitor::computeAddress(IdentifierNode *e) {
    if (lvalTarget) {
        lvalTarget->kind = LValKind::Id;
        lvalTarget->name = e->name;
        return;
    }
    leaVar(e->name);
}

void GenCodeVisitor::computeAddress(IndexNode *e) {
    e->index->accept(this);
    out << "  movq %rax, %rdi\n";
    if (auto *id = dynamic_cast<IdentifierNode *>(e->base))
        loadVar(id->name);
    out << "  leaq (%rax, %rdi, 8), %rax\n";
}

void GenCodeVisitor::computeAddress(MemberAccessNode *e) {
    if (auto *id = dynamic_cast<IdentifierNode *>(e->object)) {
        leaVar(id->name);
        out << "  addq $" << structFieldOffset[id->name][e->member] << ", %rax\n";
    }
}

void GenCodeVisitor::computeAddress(ArrowAccessNode *e) {
    if (auto *id = dynamic_cast<IdentifierNode *>(e->pointer)) {
        loadVar(id->name);
        out << "  addq $" << structFieldOffset[id->name][e->member] << ", %rax\n";
    }
}
