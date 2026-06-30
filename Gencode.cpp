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
void FcallNode::accept(CodeGenVisitor* v) { v->visit(this); }
void IndexNode::accept(CodeGenVisitor* v) { v->visit(this); }
void MemberAccessNode::accept(CodeGenVisitor* v) { v->visit(this); }
void ArrowAccessNode::accept(CodeGenVisitor* v) { v->visit(this); }
void MallocNode::accept(CodeGenVisitor* v) { v->visit(this); }
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

static string baseStructName(const string& structName) {
    size_t ltPos = structName.find('<');
    if (ltPos != string::npos) {
        return structName.substr(0, ltPos);
    }
    return structName;
}

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

    // Visit struct declarations first to initialize structFieldOffset
    for (auto s : p->structs)
        s->accept(this);

    for (auto t : p->templates)
        t->accept(this);

    out << ".data\n";
    out << "print_fmt: .string \"%ld\\n\"\n";
    out << "println_fmt: .string \"\\n\"\n";

    for (auto &[name, _] : memoriaGlobal)
        out << name << ": .quad 0\n";

    out << "\n.text\n";
    out << ".globl main\n";

    for (auto f : p->functions)
        f->accept(this);

    for (auto f : p->instantiated_functions)
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

// Helper: sufijo de instrucción según tamaño
string GenCodeVisitor::instrSuffix(int size) {
    switch (size) {
        case 1: return "b";
        case 4: return "l";
        case 8: return "q";
        default: return "q";
    }
}

// Helper: instrucción de carga según tamaño
string GenCodeVisitor::loadInstr(int size) {
    switch (size) {
        case 1: return "movzbq";  // zero-extend 1 byte -> 8 bytes
        case 4: return "movslq";  // sign-extend 4 bytes -> 8 bytes
        case 8: return "movq";    // 8 bytes (punteros, double)
        default: return "movq";
    }
}

// Helper: instrucción de almacenamiento según tamaño
string GenCodeVisitor::storeInstr(int size) {
    switch (size) {
        case 1: return "movb";
        case 4: return "movl";
        case 8: return "movq";
        default: return "movq";
    }
}

static bool isFloatSemanticType(Type* t) {
    return t && (t->ttype == Type::FLOAT || t->ttype == Type::DOUBLE);
}

string GenCodeVisitor::bindingMem(VarDecl* vd) const {
    if (!vd)
        throw runtime_error("Variable sin binding en codegen");
    if (memoriaGlobal.count(vd->name))
        return vd->name + "(%rip)";
    if (funcName.rfind(".Llambda_", 0) == 0 && memoria.count(vd->name))
        return to_string(memoria.at(vd->name)) + "(%rbp)";
    return to_string(vd->offset) + "(%rbp)";
}

void GenCodeVisitor::bind_var_decl(VarDecl* vd) {
    if (!vd) return;
    memoria[vd->name] = vd->offset;
}

int GenCodeVisitor::array_elem_size(VarDecl* vd) const {
    if (!vd || !vd->resolvedType) return 8;
    Type* t = vd->resolvedType;
    if (t->ttype == Type::ARRAY) {
        Type* base = t;
        while (base && base->ttype == Type::ARRAY)
            base = ((ArrayType*)base)->base;
        if (base) return base->size();
    }
    if (t->ttype == Type::POINTER && ((PointerType*)t)->base)
        return ((PointerType*)t)->base->size();
    return 8;
}

static vector<int> arrayDimsFor(VarDecl* vd) {
    if (!vd || !vd->resolvedType) return {};
    vector<int> dims;
    Type* t = vd->resolvedType;
    while (t && t->ttype == Type::ARRAY) {
        ArrayType* at = (ArrayType*)t;
        if (at->length > 0) dims.push_back(at->length);
        t = at->base;
    }
    return dims;
}

static void collectIndices(Exp* e, vector<Exp*>& indices, Exp*& base) {
    Exp* b = e;
    while (auto* inner = dynamic_cast<IndexNode*>(b)) {
        indices.push_back(inner->index);
        b = inner->base;
    }
    reverse(indices.begin(), indices.end());
    base = b;
}

void GenCodeVisitor::loadBinding(VarDecl* vd) {
    if (!vd) return;
    int size = vd->memSize > 0 ? vd->memSize : 8;
    Type* type = vd->resolvedType;
    if (isFloatSemanticType(type)) {
        if (type->ttype == Type::DOUBLE) {
            out << "  movsd " << bindingMem(vd) << ", %xmm7\n";
            out << "  movq %xmm7, %rax\n";
        } else {
            out << "  movss " << bindingMem(vd) << ", %xmm7\n";
            out << "  movd %xmm7, %eax\n";
            out << "  movslq %eax, %rax\n";
        }
        return;
    }
    out << "  " << loadInstr(size) << " " << bindingMem(vd) << ", %rax\n";
}

void GenCodeVisitor::storeBinding(VarDecl* vd) {
    if (!vd) return;
    int size = vd->memSize > 0 ? vd->memSize : 8;
    Type* type = vd->resolvedType;
    if (isFloatSemanticType(type)) {
        if (type->ttype == Type::DOUBLE) {
            out << "  movq %rax, %xmm7\n";
            out << "  movsd %xmm7, " << bindingMem(vd) << "\n";
        } else {
            out << "  movd %eax, %xmm7\n";
            out << "  movss %xmm7, " << bindingMem(vd) << "\n";
        }
        return;
    }
    string reg = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";
    out << "  " << storeInstr(size) << " " << reg << ", " << bindingMem(vd) << "\n";
}

void GenCodeVisitor::leaBinding(VarDecl* vd) {
    if (!vd) return;
    out << "  leaq " << bindingMem(vd) << ", %rax\n";
}

void GenCodeVisitor::emitIndexedAddress(VarDecl* vd, const vector<Exp*>& indices) {
    if (!vd || indices.empty()) return;
    auto dims = arrayDimsFor(vd);
    int elemSize = array_elem_size(vd);

    if (dims.size() > 1 && indices.size() > 1) {
        leaBinding(vd);
        out << "  movq %rax, %r8\n";
        out << "  movq $0, %r10\n";
        for (size_t d = 0; d < indices.size(); d++) {
            indices[d]->accept(this);
            int stride = 1;
            for (size_t s = d + 1; s < dims.size(); s++)
                stride *= dims[s];
            if (stride != 1) {
                out << "  movq $" << stride << ", %rcx\n";
                out << "  imulq %rcx, %rax\n";
            }
            out << "  addq %rax, %r10\n";
        }
        out << "  movq %r8, %rax\n";
        if (elemSize == 1) out << "  addq %r10, %rax\n";
        else if (elemSize == 2) out << "  leaq (%rax,%r10,2), %rax\n";
        else if (elemSize == 4) out << "  leaq (%rax,%r10,4), %rax\n";
        else out << "  leaq (%rax,%r10,8), %rax\n";
        return;
    }

    indices.back()->accept(this);
    out << "  movq %rax, %rdi\n";
    if (!dims.empty())
        leaBinding(vd);
    else
        loadBinding(vd);

    string scale;
    if (elemSize == 1) scale = "";
    else if (elemSize == 2) scale = ",2";
    else if (elemSize == 4) scale = ",4";
    else scale = ",8";
    out << "  leaq (%rax,%rdi" << scale << "), %rax\n";
}

void GenCodeVisitor::emitIndexedLoad(VarDecl* vd, const vector<Exp*>& indices) {
    if (!vd || indices.empty()) return;
    auto dims = arrayDimsFor(vd);
    int elemSize = array_elem_size(vd);

    if (dims.size() > 1 && indices.size() > 1) {
        emitIndexedAddress(vd, indices);
        if (elemSize == 1) out << "  movzbq (%rax), %rax\n";
        else if (elemSize == 4) out << "  movslq (%rax), %rax\n";
        else out << "  movq (%rax), %rax\n";
        return;
    }

    indices.back()->accept(this);
    out << "  movq %rax, %rdi\n";
    if (!dims.empty())
        leaBinding(vd);
    else
        loadBinding(vd);

    string scale;
    if (elemSize == 1) scale = "";
    else if (elemSize == 2) scale = ",2";
    else if (elemSize == 4) scale = ",4";
    else scale = ",8";
    out << "  " << loadInstr(elemSize) << " (%rax,%rdi" << scale << "), %rax\n";
}

void GenCodeVisitor::emitIndexedStore(VarDecl* vd, const vector<Exp*>& indices,
                                      const string& valueReg) {
    if (!vd || indices.empty()) return;
    auto dims = arrayDimsFor(vd);
    int elemSize = array_elem_size(vd);
    string reg = valueReg;
    if (elemSize == 1 && valueReg == "%rax") reg = "%al";
    else if (elemSize == 4 && valueReg == "%rax") reg = "%eax";

    if (dims.size() > 1 && indices.size() > 1) {
        out << "  movq %rax, %r9\n";
        emitIndexedAddress(vd, indices);
        out << "  movq %rax, %rbx\n";
        string valReg = "%r9";
        if (elemSize == 1) valReg = "%r9b";
        else if (elemSize == 4) valReg = "%r9d";
        out << "  " << storeInstr(elemSize) << " " << valReg << ", (%rbx)\n";
        return;
    }

    out << "  movq %rax, %rcx\n";
    indices.back()->accept(this);
    out << "  movq %rax, %rdi\n";
    if (!dims.empty())
        leaBinding(vd);
    else
        loadBinding(vd);

    string valReg = "%rcx";
    if (elemSize == 1) valReg = "%cl";
    else if (elemSize == 4) valReg = "%ecx";

    string scale;
    if (elemSize == 1) scale = "";
    else if (elemSize == 2) scale = ",2";
    else if (elemSize == 4) scale = ",4";
    else scale = ",8";
    out << "  " << storeInstr(elemSize) << " " << valReg << ", (%rax,%rdi" << scale << ")\n";
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
        storeBinding(lv.binding);
        break;
    case LValKind::Index:
    {
        vector<Exp*> indices = lv.indices;
        if (indices.empty() && lv.index)
            indices.push_back(lv.index);
        emitIndexedStore(lv.binding, indices, "%rax");
        break;
    }
    case LValKind::Member:
    {
        out << "  movq %rax, %rcx\n";
        int size = structMemberSizes[baseStructName(lv.structName)][lv.member];
        string reg = (size == 1) ? "%cl" : (size == 4) ? "%ecx" : "%rcx";
        if (lv.isArrow) {
            loadBinding(lv.binding);
            out << "  " << storeInstr(size) << " " << reg << ", " << structFieldOffset[baseStructName(lv.structName)][lv.member] << "(%rax)\n";
        } else {
            leaBinding(lv.binding);
            out << "  " << storeInstr(size) << " " << reg << ", " << structFieldOffset[baseStructName(lv.structName)][lv.member] << "(%rax)\n";
        }
        break;
    }
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
    union { float f; unsigned int i; } fc;
    fc.f = (float)e->value;
    out << "  movl $0x" << hex << fc.i << dec << ", %eax\n";
    out << "  movslq %eax, %rax\n";
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
        lvalTarget->binding = e->binding;
        return;
    }
    loadBinding(e->binding);
}

void GenCodeVisitor::visit(BinaryOpNode *e) {
    // Constant folding: si es constante, generar directamente el valor
    if (e->isConstant) {
        out << "  movq $" << (long long)e->constantValue << ", %rax\n";
        return;
    }
    
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

    Type* leftType = e->left->resolvedType;
    Type* rightType = e->right->resolvedType;
    bool useFloat = false;
    bool useDouble = false;
    switch (e->op) {
        case BinaryOp::ADD: case BinaryOp::SUB: case BinaryOp::MUL: case BinaryOp::DIV:
        case BinaryOp::EQ: case BinaryOp::NE: case BinaryOp::LT: case BinaryOp::GT:
        case BinaryOp::LE: case BinaryOp::GE:
            if (isFloatSemanticType(leftType) || isFloatSemanticType(rightType)) {
                useFloat = true;
                useDouble = (leftType && leftType->ttype == Type::DOUBLE) ||
                            (rightType && rightType->ttype == Type::DOUBLE) ||
                            (e->resolvedType && e->resolvedType->ttype == Type::DOUBLE);
            }
            break;
        default:
            break;
    }

    if (useFloat) {
        e->left->accept(this);
        if (useDouble) {
            if (!isFloatSemanticType(leftType))
                out << "  cvtsi2sd %rax, %xmm0\n";
            else if (leftType->ttype == Type::FLOAT) {
                out << "  movd %eax, %xmm0\n";
                out << "  cvtss2sd %xmm0, %xmm0\n";
            } else
                out << "  movq %rax, %xmm0\n";
            e->right->accept(this);
            if (!isFloatSemanticType(rightType))
                out << "  cvtsi2sd %rax, %xmm1\n";
            else if (rightType->ttype == Type::FLOAT) {
                out << "  movd %eax, %xmm1\n";
                out << "  cvtss2sd %xmm1, %xmm1\n";
            } else
                out << "  movq %rax, %xmm1\n";
        } else {
            if (!isFloatSemanticType(leftType))
                out << "  cvtsi2ss %eax, %xmm0\n";
            else
                out << "  movd %eax, %xmm0\n";
            e->right->accept(this);
            if (!isFloatSemanticType(rightType))
                out << "  cvtsi2ss %eax, %xmm1\n";
            else
                out << "  movd %eax, %xmm1\n";
        }

        switch (e->op) {
        case BinaryOp::ADD:
            out << (useDouble ? "  addsd %xmm1, %xmm0\n" : "  addss %xmm1, %xmm0\n");
            break;
        case BinaryOp::SUB:
            out << (useDouble ? "  subsd %xmm1, %xmm0\n" : "  subss %xmm1, %xmm0\n");
            break;
        case BinaryOp::MUL:
            out << (useDouble ? "  mulsd %xmm1, %xmm0\n" : "  mulss %xmm1, %xmm0\n");
            break;
        case BinaryOp::DIV:
            out << (useDouble ? "  divsd %xmm1, %xmm0\n" : "  divss %xmm1, %xmm0\n");
            break;
        case BinaryOp::EQ:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  sete %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::NE:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  setne %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::LT:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  setb %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::GT:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  seta %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::LE:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  setbe %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        case BinaryOp::GE:
            out << (useDouble ? "  ucomisd %xmm1, %xmm0\n" : "  ucomiss %xmm1, %xmm0\n");
            out << "  movq $0, %rax\n";
            out << "  setae %al\n";
            out << "  movzbq %al, %rax\n";
            return;
        default:
            break;
        }

        if (useDouble)
            out << "  movq %xmm0, %rax\n";
        else {
            out << "  movd %xmm0, %eax\n";
            out << "  movslq %eax, %rax\n";
        }
        return;
    }

    e->left->accept(this);
    out << "  pushq %rax\n";
    e->right->accept(this);
    out << "  movq %rax, %rcx\n";
    out << "  popq %rax\n";

    // Determinar tamaño de operación según resolvedType
    int size = 8;  // default
    if (e->resolvedType) {
        size = e->resolvedType->size();
    }
    
    string suffix = (size == 1) ? "b" : (size == 4) ? "l" : "q";
    string reg1 = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";
    string reg2 = (size == 1) ? "%cl" : (size == 4) ? "%ecx" : "%rcx";

    switch (e->op) {
    case BinaryOp::ADD: out << "  add" << suffix << " " << reg2 << ", " << reg1 << "\n"; break;
    case BinaryOp::SUB: out << "  sub" << suffix << " " << reg2 << ", " << reg1 << "\n"; break;
    case BinaryOp::MUL: out << "  imul" << suffix << " " << reg2 << ", " << reg1 << "\n"; break;
    case BinaryOp::DIV:
        out << "  cqto\n";
        out << "  idiv" << suffix << " " << reg2 << "\n";
        break;
    case BinaryOp::MOD:
        out << "  cqto\n";
        out << "  idiv" << suffix << " " << reg2 << "\n";
        out << "  movq %rdx, %rax\n";
        break;
    case BinaryOp::POW:
        out << "  movq %rax, %rdi\n";
        out << "  movq %rcx, %rsi\n";
        out << "  call potencia\n";
        break;
    case BinaryOp::EQ:
        out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
        out << "  movq $0, %rax\n";
        out << "  sete %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::NE:
        out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
        out << "  movq $0, %rax\n";
        out << "  setne %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::LT:
        out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
        out << "  movq $0, %rax\n";
        out << "  setl %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::GT:
        out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
        out << "  movq $0, %rax\n";
        out << "  setg %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::LE:
        out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
        out << "  movq $0, %rax\n";
        out << "  setle %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::GE:
        out << "  cmp" << suffix << " " << reg2 << ", " << reg1 << "\n";
        out << "  movq $0, %rax\n";
        out << "  setge %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case BinaryOp::LOG_AND:
        out << "  and" << suffix << " " << reg2 << ", " << reg1 << "\n";
        break;
    case BinaryOp::LOG_OR:
        out << "  or" << suffix << " " << reg2 << ", " << reg1 << "\n";
        break;
    }
}

void GenCodeVisitor::visit(UnaryOpNode *e) {
    // lvalue mode for deref: *p = value
    if (lvalTarget && e->op == UnaryOp::DEREF) {
        lvalTarget->kind = LValKind::Deref;
        lvalTarget = nullptr;
        e->operand->accept(this);
        out << "  pushq %rax\n";
        return;
    }
    
    // Constant folding: si es constante, generar directamente el valor
    if (e->isConstant) {
        out << "  movq $" << (long long)e->constantValue << ", %rax\n";
        return;
    }
    
    e->operand->accept(this);

    int size = e->resolvedType ? e->resolvedType->size() : 8;
    
    string suffix = (size == 1) ? "b" : (size == 4) ? "l" : "q";
    string reg = (size == 1) ? "%al" : (size == 4) ? "%eax" : "%rax";

    switch (e->op) {
    case UnaryOp::MINUS:
        out << "  neg" << suffix << " " << reg << "\n";
        break;
    case UnaryOp::LOG_NOT:
        out << "  cmpq $0, %rax\n";
        out << "  movq $0, %rax\n";
        out << "  sete %al\n";
        out << "  movzbq %al, %rax\n";
        break;
    case UnaryOp::PRE_INC:
        out << "  inc" << suffix << " " << reg << "\n";
        if (auto *id = dynamic_cast<IdentifierNode *>(e->operand))
            storeBinding(id->binding);
        break;
    case UnaryOp::PRE_DEC:
        out << "  dec" << suffix << " " << reg << "\n";
        if (auto *id = dynamic_cast<IdentifierNode *>(e->operand))
            storeBinding(id->binding);
        break;
    case UnaryOp::POST_INC:
        if (auto *id = dynamic_cast<IdentifierNode *>(e->operand)) {
            out << "  pushq %rax\n";
            out << "  inc" << suffix << " " << reg << "\n";
            storeBinding(id->binding);
            out << "  popq %rax\n";
        }
        break;
    case UnaryOp::POST_DEC:
        if (auto *id = dynamic_cast<IdentifierNode *>(e->operand)) {
            out << "  pushq %rax\n";
            out << "  dec" << suffix << " " << reg << "\n";
            storeBinding(id->binding);
            out << "  popq %rax\n";
        }
        break;
    case UnaryOp::ADDR:
        if (auto *id = dynamic_cast<IdentifierNode *>(e->operand))
            leaBinding(id->binding);
        break;
    case UnaryOp::DEREF:
        out << "  movq (%rax), %rax\n";
        break;
    }
}

void GenCodeVisitor::visit(AssignmentNode *e) {
    LVal target = captureLVal(e->target);

    e->value->accept(this); // value in %rax

    if (target.kind == LValKind::Id && target.binding) {
        Type* tgtType = target.binding->resolvedType;
        Type* valType = e->value->resolvedType;
        if (tgtType && tgtType->ttype == Type::DOUBLE) {
            if (!valType || valType->ttype == Type::FLOAT || valType->ttype == Type::INT ||
                valType->ttype == Type::CHAR) {
                out << "  movd %eax, %xmm7\n";
                out << "  cvtss2sd %xmm7, %xmm7\n";
                out << "  movq %xmm7, %rax\n";
            }
        } else if (tgtType && tgtType->ttype == Type::FLOAT) {
            if (!valType || valType->ttype == Type::INT || valType->ttype == Type::CHAR) {
                out << "  cvtsi2ss %eax, %xmm7\n";
                out << "  movd %xmm7, %eax\n";
                out << "  movslq %eax, %rax\n";
            }
        }
    }

    switch (e->op) {
    case AssignOp::ASSIGN:
        break;
    }

    storeTarget(target);
}

void GenCodeVisitor::visit(PrintfNode *e) {
    // Generar label para el formato (respetar el formato del usuario)
    string fmtLabel = "print_fmt";
    if (e->format != "%ld") {
        auto it = stringLabels.find(e->format);
        int lbl;
        if (it == stringLabels.end()) {
            lbl = (int)stringLabels.size();
            stringLabels[e->format] = lbl;
            out << ".section .rodata\n";
            out << ".Lfmt" << lbl << ": .string \"" << e->format << "\"\n";
            out << ".text\n";
            fmtLabel = ".Lfmt" + to_string(lbl);
        } else {
            fmtLabel = ".Lfmt" + to_string(it->second);
        }
    }
    
    // Cargar argumentos en registros según convención System V AMD64 ABI
    // Para printf: formato en %rdi, luego argumentos en orden
    // Cada posición tiene su registro específico:
    // Posición 0: rsi (int) o xmm0 (float)
    // Posición 1: rdx (int) o xmm1 (float)
    // Posición 2: rcx (int) o xmm2 (float)
    // Posición 3: r8 (int) o xmm3 (float)
    // Posición 4: r9 (int) o xmm4 (float)
    
    const vector<string> intRegs = {"%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    const vector<string> xmmRegs = {"%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7"};
    
    for (size_t i = 0; i < e->args.size(); i++) {
        e->args[i]->accept(this);
        
        // Determinar si es float/double usando TypeChecker
        bool isFloat = false;
        if (e->args[i]->resolvedType) {
            Type* t = e->args[i]->resolvedType;
            isFloat = (t->ttype == Type::FLOAT || t->ttype == Type::DOUBLE);
        }
        
        if (isFloat && i < xmmRegs.size()) {
            // Mover float/double a xmm register usando movq
            out << "  movq %rax, " << xmmRegs[i] << "\n";
        } else if (i < intRegs.size()) {
            // Mover entero/puntero a general purpose register
            out << "  movq %rax, " << intRegs[i] << "\n";
        } else {
            // Push al stack si no hay registros disponibles
            out << "  pushq %rax\n";
        }
    }
    
    // Llamar a printf con el formato en %rdi
    out << "  leaq " << fmtLabel << "(%rip), %rdi\n";
    
    // Configurar al para variadic functions (número de registros xmm usados)
    int xmmCount = 0;
    for (size_t i = 0; i < e->args.size() && i < xmmRegs.size(); i++) {
        if (e->args[i]->resolvedType) {
            Type* t = e->args[i]->resolvedType;
            if (t->ttype == Type::FLOAT || t->ttype == Type::DOUBLE) {
                xmmCount++;
            }
        }
    }
    out << "  movq $" << xmmCount << ", %rax\n";
    
    out << "  call printf@PLT\n";
    
    // Limpiar argumentos extra del stack
    int regsUsed = min((int)e->args.size(), (int)max(intRegs.size(), xmmRegs.size()));
    if ((int)e->args.size() > regsUsed) {
        out << "  addq $" << ((e->args.size() - regsUsed) * 8) << ", %rsp\n";
    }
    
    out << "  movq $0, %rax\n";
}

void GenCodeVisitor::visit(FcallNode *e) {
    const vector<string> argRegs = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    auto *id = dynamic_cast<IdentifierNode *>(e->callee);
    string fname = id ? id->name : "";

    int nArgs = (int)e->args.size();
    
    // Check if calling a lambda (stored in memoria)
    bool isLambdaCall = !fname.empty() && memoria.count(fname);
    
    // For lambdas, pass current frame pointer in %rdi as first hidden argument
    if (isLambdaCall) {
        out << "  movq %rbp, %rdi\n";
    }
    
    // Primero, push argumentos extra (>6) en orden inverso
    for (int i = nArgs - 1; i >= 6; i--) {
        e->args[i]->accept(this);
        out << "  pushq %rax\n";
    }
    
    // Luego, cargar los primeros 6 argumentos en registros
    // For lambdas, start from %rsi since %rdi is used for frame pointer
    int argRegOffset = isLambdaCall ? 1 : 0;
    for (int i = 0; i < nArgs && i < 6; i++) {
        e->args[i]->accept(this);
        out << "  movq %rax, " << argRegs[i + argRegOffset] << "\n";
    }
    
    if (!fname.empty() && memoria.count(fname)) {
        out << "  call *" << memoria[fname] << "(%rbp)\n";
    } else {
        out << "  call " << fname << "\n";
    }
    
    // Limpiar argumentos extra del stack
    if (nArgs > 6) {
        out << "  addq $" << ((nArgs - 6) * 8) << ", %rsp\n";
    }
}

void GenCodeVisitor::visit(IndexNode *e) {
    if (lvalTarget) {
        lvalTarget->kind = LValKind::Index;
        vector<Exp*> indices;
        Exp* b = nullptr;
        collectIndices(e, indices, b);
        if (auto *id = dynamic_cast<IdentifierNode *>(b)) {
            lvalTarget->name = id->name;
            lvalTarget->binding = id->binding;
        }
        lvalTarget->indices = indices;
        if (!indices.empty())
            lvalTarget->index = indices.back();
        return;
    }

    vector<Exp*> indices;
    Exp* b = nullptr;
    collectIndices(e, indices, b);

    VarDecl* arrBinding = nullptr;
    if (auto* id = dynamic_cast<IdentifierNode*>(b))
        arrBinding = id->binding;

    emitIndexedLoad(arrBinding, indices);
}

void GenCodeVisitor::visit(MemberAccessNode *e) {
    if (lvalTarget) {
        if (auto *id = dynamic_cast<IdentifierNode *>(e->object)) {
            lvalTarget->kind = LValKind::Member;
            lvalTarget->name = id->name;
            lvalTarget->binding = id->binding;
            lvalTarget->member = e->member;
            lvalTarget->isArrow = false;
            Type* objType = e->object->resolvedType;
            if (!objType && id->binding)
                objType = id->binding->resolvedType;
            if (objType && objType->ttype == Type::STRUCT) {
                lvalTarget->structName = ((StructType*)objType)->name;
            } else {
                lvalTarget->structName = id->name;
            }
        }
        return;
    }
    if (auto *id = dynamic_cast<IdentifierNode *>(e->object)) {
        leaBinding(id->binding);
        string structName;
        Type* objType = id->resolvedType;
        if (objType && objType->ttype == Type::STRUCT) {
            structName = ((StructType*)objType)->name;
        } else if (id->binding && id->binding->resolvedType &&
                   id->binding->resolvedType->ttype == Type::STRUCT) {
            structName = ((StructType*)id->binding->resolvedType)->name;
        } else {
            structName = id->name;
        }
        int size = structMemberSizes[baseStructName(structName)][e->member];
        out << "  " << loadInstr(size) << " " << structFieldOffset[baseStructName(structName)][e->member] << "(%rax), %rax\n";
    }
}

void GenCodeVisitor::visit(ArrowAccessNode *e) {
    if (lvalTarget) {
        lvalTarget->kind = LValKind::Member;
        if (auto *id = dynamic_cast<IdentifierNode *>(e->pointer)) {
            lvalTarget->name = id->name;
            lvalTarget->binding = id->binding;
            // Get the struct type name from resolvedType (should be pointer to struct)
            if (id->resolvedType && id->resolvedType->ttype == Type::POINTER) {
                PointerType* pt = (PointerType*)id->resolvedType;
                if (pt->base && pt->base->ttype == Type::STRUCT) {
                    lvalTarget->structName = ((StructType*)pt->base)->name;
                } else {
                    lvalTarget->structName = id->name;
                }
            } else {
                lvalTarget->structName = id->name;
            }
        }
        lvalTarget->member = e->member;
        lvalTarget->isArrow = true;
        return;
    }
    if (auto *id = dynamic_cast<IdentifierNode *>(e->pointer)) {
        loadBinding(id->binding);
        string structName;
        // Get the struct type name from resolvedType (should be pointer to struct)
        if (id->resolvedType && id->resolvedType->ttype == Type::POINTER) {
            PointerType* pt = (PointerType*)id->resolvedType;
            if (pt->base && pt->base->ttype == Type::STRUCT) {
                structName = ((StructType*)pt->base)->name;
            } else {
                structName = id->name;
            }
        } else {
            structName = id->name;
        }
        int size = structMemberSizes[baseStructName(structName)][e->member];
        out << "  " << loadInstr(size) << " " << structFieldOffset[baseStructName(structName)][e->member] << "(%rax), %rax\n";
    }
}

void GenCodeVisitor::visit(MallocNode *e) {
    e->size->accept(this);
    out << "  movq %rax, %rdi\n";
    out << "  call malloc@PLT\n";
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
    else if (auto *st = dynamic_cast<StructTypeNode *>(e->target_type)) {
        // Calcular tamaño del struct sumando tamaños de miembros
        auto it = structFieldCount.find(st->name);
        if (it != structFieldCount.end()) {
            out << "  movq $" << (it->second * 8) << ", %rax\n";
        } else {
            out << "  movq $0, %rax\n";
        }
    }
    else if (auto *nt = dynamic_cast<NamedTypeNode *>(e->target_type)) {
        // Podría ser un struct instanciado
        auto it = structFieldCount.find(nt->name);
        if (it != structFieldCount.end()) {
            out << "  movq $" << (it->second * 8) << ", %rax\n";
        } else {
            out << "  movq $0, %rax\n";
        }
    }
    else
        out << "  movq $0, %rax\n";
}

void GenCodeVisitor::visit(PrimitiveTypeNode *) {}
void GenCodeVisitor::visit(PointerTypeNode *) {}
void GenCodeVisitor::visit(StructTypeNode *) {}
void GenCodeVisitor::visit(NamedTypeNode *) {}

void GenCodeVisitor::visit(TemplateTypeNode *) {}
void GenCodeVisitor::visit(CaptureNode *) {}

// Helper para contar variables en lambdas (no pasan por TypeChecker)
static int countLambdaVars(Stm* stmt) {
    if (!stmt) return 0;
    int count = 0;
    
    if (dynamic_cast<VarDecl*>(stmt)) count = 1;
    
    if (auto* body = dynamic_cast<Body*>(stmt)) {
        for (auto s : body->stmts) count += countLambdaVars(s);
    }
    if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        count += countLambdaVars(ifStmt->then_branch);
        if (ifStmt->else_branch) count += countLambdaVars(ifStmt->else_branch);
    }
    if (auto* whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        count += countLambdaVars(whileStmt->body);
    }
    if (auto* forStmt = dynamic_cast<ForStmt*>(stmt)) {
        if (forStmt->init) count += countLambdaVars(forStmt->init);
        count += countLambdaVars(forStmt->body);
    }
    if (auto* doWhileStmt = dynamic_cast<DoWhileStmt*>(stmt)) {
        count += countLambdaVars(doWhileStmt->body);
    }
    if (auto* switchStmt = dynamic_cast<SwitchStmt*>(stmt)) {
        for (auto cc : switchStmt->cases) {
            for (auto s : cc->body) count += countLambdaVars(s);
        }
        for (auto s : switchStmt->default_body) count += countLambdaVars(s);
    }
    return count;
}

void GenCodeVisitor::visit(LambdaExprNode *e) {
    int lbl = labelcont++;
    string lambdaName = ".Llambda_" + to_string(lbl);

    string savedFunc = funcName;
    bool savedInFunc = inFunction;
    int savedOffset = offset;
    auto savedMemoria = memoria;

    inFunction = true;
    memoria.clear();
    offset = 0;
    funcName = lambdaName;
    returnLabel = ".Llambda_end_" + to_string(lbl);

    // Calcular frame size para lambda (asume 8 bytes por variable)
    int localVarCount = countLambdaVars(e->body);
    int paramCount = (int)e->params.size();
    int captureCount = (int)e->captures.size();
    int totalVars = paramCount + localVarCount + captureCount;
    int frameSize = (totalVars * 8 + 15) & ~15;
    string afterLabel = ".Llambda_after_" + to_string(lbl);

    out << "  jmp " << afterLabel << "\n";
    out << lambdaName << ":\n";
    out << "  pushq %rbp\n";
    out << "  movq %rsp, %rbp\n";
    // Store parent frame pointer (passed in %rdi) at 0(%rbp) for captures
    out << "  movq %rdi, 0(%rbp)\n";
    if (frameSize > 0)
        out << "  subq $" << frameSize << ", %rsp\n";

    const vector<string> argRegs = {"%rsi", "%rdx", "%rcx", "%r8", "%r9", "%r10"};
    for (size_t i = 0; i < e->params.size(); i++) {
        offset -= 8;
        e->params[i]->offset = offset;
        memoria[e->params[i]->name] = offset;
        if (i < argRegs.size())
            out << "  movq " << argRegs[i] << ", " << offset << "(%rbp)\n";
    }

    for (auto cap : e->captures) {
        offset -= 8;
        memoria[cap->name] = offset;
    }

    for (auto cap : e->captures) {
        if (cap->mode == CaptureNode::BY_VALUE && savedMemoria.count(cap->name)) {
            out << "  movq 0(%rbp), %rax\n";
            out << "  movl " << savedMemoria[cap->name] << "(%rax), %eax\n";
            out << "  movl %eax, " << memoria[cap->name] << "(%rbp)\n";
        }
    }

    e->body->accept(this);

    out << ".Llambda_end_" << lbl << ":\n";
    out << "  leave\n";
    out << "  ret\n";
    out << afterLabel << ":\n";

    inFunction = savedInFunc;
    offset = savedOffset;
    memoria = savedMemoria;
    funcName = savedFunc;
    returnLabel.clear();

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
    string oldContinue = currentContinueLabel;
    currentBreakLabel = "endwhile_" + to_string(lbl);
    currentContinueLabel = "while_" + to_string(lbl);

    out << "while_" << lbl << ":\n";
    s->condition->accept(this);
    out << "  cmpq $0, %rax\n";
    out << "  je endwhile_" << lbl << "\n";
    s->body->accept(this);
    out << "  jmp while_" << lbl << "\n";
    out << "endwhile_" << lbl << ":\n";

    currentBreakLabel = oldBreak;
    currentContinueLabel = oldContinue;
}

void GenCodeVisitor::visit(DoWhileStmt *s) {
    int lbl = labelcont++;
    string oldBreak = currentBreakLabel;
    string oldContinue = currentContinueLabel;
    currentBreakLabel = "endwhile_" + to_string(lbl);
    currentContinueLabel = "docond_" + to_string(lbl);

    out << "dowhile_" << lbl << ":\n";
    s->body->accept(this);
    out << "docond_" << lbl << ":\n";
    s->condition->accept(this);
    out << "  cmpq $0, %rax\n";
    out << "  jne dowhile_" << lbl << "\n";
    out << "endwhile_" << lbl << ":\n";

    currentBreakLabel = oldBreak;
    currentContinueLabel = oldContinue;
}

void GenCodeVisitor::visit(ForStmt *s) {
    int lbl = labelcont++;
    string oldBreak = currentBreakLabel;
    string oldContinue = currentContinueLabel;
    currentBreakLabel = "endfor_" + to_string(lbl);
    currentContinueLabel = "forinc_" + to_string(lbl);

    if (s->init)
        s->init->accept(this);

    out << "for_" << lbl << ":\n";
    if (s->condition) {
        s->condition->accept(this);
        out << "  cmpq $0, %rax\n";
        out << "  je endfor_" << lbl << "\n";
    }
    s->body->accept(this);
    out << "forinc_" << lbl << ":\n";
    if (s->increment)
        s->increment->accept(this);
    out << "  jmp for_" << lbl << "\n";
    out << "endfor_" << lbl << ":\n";

    currentBreakLabel = oldBreak;
    currentContinueLabel = oldContinue;
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
    if (currentContinueLabel.empty()) {
        cerr << "Error: continue fuera de ciclo\n";
        exit(1);
    }
    out << "  jmp " << currentContinueLabel << "\n";
}

void GenCodeVisitor::visit(ReturnStmt *s) {
    if (s->expr)
        s->expr->accept(this);
    else
        out << "  movq $0, %rax\n";
    string target = returnLabel.empty() ? ".end_" + funcName : returnLabel;
    out << "  jmp " << target << "\n";
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

    bind_var_decl(d);

    if (d->initializer) {
        d->initializer->accept(this);
        if (d->resolvedType && d->resolvedType->ttype == Type::DOUBLE) {
            Type* initType = d->initializer->resolvedType;
            if (!initType || initType->ttype == Type::FLOAT || initType->ttype == Type::INT ||
                initType->ttype == Type::CHAR) {
                out << "  movd %eax, %xmm7\n";
                out << "  cvtss2sd %xmm7, %xmm7\n";
                out << "  movq %xmm7, %rax\n";
            }
        }
        storeBinding(d);
    }
}

void GenCodeVisitor::visit(FunDecl *d) {
    inFunction = true;
    memoria.clear();
    funcName = d->name;
    returnLabel = ".end_" + d->name;
    
    // Usar frameSize calculado por TypeChecker
    int frameSize = d->frameSize;

    out << "\n.globl " << d->name << "\n";
    out << d->name << ":\n";
    out << "  pushq %rbp\n";
    out << "  movq %rsp, %rbp\n";
    if (frameSize > 0)
        out << "  subq $" << frameSize << ", %rsp\n";

    const vector<string> argRegs32 = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    const vector<string> argRegs64 = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    for (size_t i = 0; i < d->params.size() && i < 6; i++) {
        bind_var_decl(d->params[i]);
        
        int size = d->params[i]->memSize;
        if (size == 4)
            out << "  movl " << argRegs32[i] << ", " << d->params[i]->offset << "(%rbp)\n";
        else if (size == 1)
            out << "  movb " << argRegs32[i] << ", " << d->params[i]->offset << "(%rbp)\n";
        else
            out << "  movq " << argRegs64[i] << ", " << d->params[i]->offset << "(%rbp)\n";
    }

    // Generar código del body
    d->body->accept(this);

    out << ".end_" << d->name << ":\n";
    out << "  leave\n";
    out << "  ret\n";
    inFunction = false;
}

void GenCodeVisitor::visit(StructDecl *d) {
    // Use the memberOffsets already calculated by TypeChecker!
    for (auto& pair : d->memberOffsets) {
        structFieldOffset[d->name][pair.first] = pair.second;
    }
    for (auto& pair : d->memberSizes) {
        structMemberSizes[d->name][pair.first] = pair.second;
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
            int msize = m->memSize > 0 ? m->memSize : 4;
            structFieldOffset[d->struct_decl->name][m->name] = fieldOff;
            structMemberSizes[d->struct_decl->name][m->name] = msize;
            fieldOff += msize;
        }
        structFieldCount[d->struct_decl->name] = (int)d->struct_decl->members.size();
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
        lvalTarget->binding = e->binding;
        return;
    }
    leaBinding(e->binding);
}

void GenCodeVisitor::computeAddress(IndexNode *e) {
    vector<Exp*> indices;
    Exp* b = nullptr;
    collectIndices(e, indices, b);

    VarDecl* arrBinding = nullptr;
    if (auto *id = dynamic_cast<IdentifierNode *>(b))
        arrBinding = id->binding;

    emitIndexedAddress(arrBinding, indices);
}

void GenCodeVisitor::computeAddress(MemberAccessNode *e) {
    if (auto *id = dynamic_cast<IdentifierNode *>(e->object)) {
        leaBinding(id->binding);
        string structName = id->name;
        if (id->resolvedType && id->resolvedType->ttype == Type::STRUCT) {
            structName = ((StructType*)id->resolvedType)->name;
        }
        out << "  addq $" << structFieldOffset[baseStructName(structName)][e->member] << ", %rax\n";
    }
}

void GenCodeVisitor::computeAddress(ArrowAccessNode *e) {
    if (auto *id = dynamic_cast<IdentifierNode *>(e->pointer)) {
        loadBinding(id->binding);
        string structName = id->name;
        if (id->resolvedType && id->resolvedType->ttype == Type::POINTER) {
            PointerType* pt = (PointerType*)id->resolvedType;
            if (pt->base && pt->base->ttype == Type::STRUCT) {
                structName = ((StructType*)pt->base)->name;
            }
        }
        out << "  addq $" << structFieldOffset[baseStructName(structName)][e->member] << ", %rax\n";
    }
}
