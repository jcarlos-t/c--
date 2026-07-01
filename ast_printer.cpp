#include "ast_printer.h"
#include <iostream>

using namespace std;

static void indent(ostream& out, int level) {
    for (int i = 0; i < level; i++) out << "  ";
}

static const char* typeNodeName(Exp* t) {
    if (auto* p = dynamic_cast<PrimitiveTypeNode*>(t)) {
        switch (p->prim) {
            case PrimitiveTypeNode::VOID: return "void";
            case PrimitiveTypeNode::INT: return "int";
            case PrimitiveTypeNode::CHAR: return "char";
            case PrimitiveTypeNode::FLOAT: return "float";
            case PrimitiveTypeNode::DOUBLE: return "double";
            case PrimitiveTypeNode::BOOL: return "bool";
            case PrimitiveTypeNode::AUTO: return "auto";
        }
    }
    if (dynamic_cast<PointerTypeNode*>(t)) return "pointer";
    if (auto* s = dynamic_cast<StructTypeNode*>(t)) return s->name.c_str();
    if (auto* n = dynamic_cast<NamedTypeNode*>(t)) return n->name.c_str();
    if (dynamic_cast<TemplateTypeNode*>(t)) return "template_type";
    return "unknown";
}

static const char* binaryOpName(BinaryOp op) {
    switch (op) {
        case BinaryOp::ADD: return "Add";
        case BinaryOp::SUB: return "Sub";
        case BinaryOp::MUL: return "Mul";
        case BinaryOp::DIV: return "Div";
        case BinaryOp::MOD: return "Mod";
        case BinaryOp::POW: return "Pow";
        case BinaryOp::EQ:  return "Eq";
        case BinaryOp::NE:  return "Ne";
        case BinaryOp::LT:  return "Lt";
        case BinaryOp::GT:  return "Gt";
        case BinaryOp::LE:  return "Le";
        case BinaryOp::GE:  return "Ge";
        case BinaryOp::LOG_AND: return "And";
        case BinaryOp::LOG_OR:  return "Or";
    }
    return "unknown";
}

static const char* unaryOpName(UnaryOp op) {
    switch (op) {
        case UnaryOp::ADDR:     return "Addr";
        case UnaryOp::DEREF:    return "Deref";
        case UnaryOp::MINUS:    return "Minus";
        case UnaryOp::LOG_NOT:  return "Not";
        case UnaryOp::PRE_INC:  return "PreInc";
        case UnaryOp::PRE_DEC:  return "PreDec";
        case UnaryOp::POST_INC: return "PostInc";
        case UnaryOp::POST_DEC: return "PostDec";
    }
    return "unknown";
}

static void printExp(ostream& out, Exp* e, int level);
static void printStm(ostream& out, Stm* s, int level);
static void printVarDecl(ostream& out, VarDecl* vd, int level);

static void printExp(ostream& out, Exp* e, int level) {
    if (!e) return;

    if (auto* bop = dynamic_cast<BinaryOpNode*>(e)) {
        indent(out, level);
        out << binaryOpName(bop->op) << "\n";
        printExp(out, bop->left, level + 1);
        printExp(out, bop->right, level + 1);
        return;
    }

    if (auto* uop = dynamic_cast<UnaryOpNode*>(e)) {
        indent(out, level);
        out << unaryOpName(uop->op) << "\n";
        printExp(out, uop->operand, level + 1);
        return;
    }

    if (auto* asn = dynamic_cast<AssignmentNode*>(e)) {
        indent(out, level);
        out << "Assign\n";
        printExp(out, asn->target, level + 1);
        printExp(out, asn->value, level + 1);
        return;
    }

    if (auto* fcall = dynamic_cast<FcallNode*>(e)) {
        indent(out, level);
        if (auto* id = dynamic_cast<IdentifierNode*>(fcall->callee)) {
            out << "Fcall: " << id->name << "\n";
        } else {
            out << "Fcall\n";
        }
        for (auto arg : fcall->args) {
            printExp(out, arg, level + 1);
        }
        return;
    }

    if (auto* idx = dynamic_cast<IndexNode*>(e)) {
        indent(out, level);
        out << "Index\n";
        printExp(out, idx->base, level + 1);
        printExp(out, idx->index, level + 1);
        return;
    }

    if (auto* mem = dynamic_cast<MemberAccessNode*>(e)) {
        indent(out, level);
        out << "MemberAccess: " << mem->member << "\n";
        printExp(out, mem->object, level + 1);
        return;
    }

    if (auto* arrow = dynamic_cast<ArrowAccessNode*>(e)) {
        indent(out, level);
        out << "ArrowAccess: " << arrow->member << "\n";
        printExp(out, arrow->pointer, level + 1);
        return;
    }

    if (auto* malloc = dynamic_cast<MallocNode*>(e)) {
        indent(out, level);
        out << "Malloc\n";
        printExp(out, malloc->size, level + 1);
        return;
    }

    if (auto* sz = dynamic_cast<SizeOfNode*>(e)) {
        indent(out, level);
        out << "SizeOf\n";
        printExp(out, sz->target_type, level + 1);
        return;
    }

    if (auto* lambda = dynamic_cast<LambdaExprNode*>(e)) {
        indent(out, level);
        out << "Lambda\n";
        for (auto cap : lambda->captures) {
            indent(out, level + 1);
            out << "Capture";
            if (cap->mode == CaptureNode::BY_REF) out << "(ref)";
            out << ": " << cap->name << "\n";
        }
        for (auto param : lambda->params) {
            printVarDecl(out, param, level + 1);
        }
        indent(out, level + 1);
        out << "-> " << typeNodeName(lambda->return_type) << "\n";
        printStm(out, lambda->body, level + 1);
        return;
    }

    if (auto* id = dynamic_cast<IdentifierNode*>(e)) {
        indent(out, level);
        out << "Identifier: " << id->name << "\n";
        return;
    }

    if (auto* il = dynamic_cast<IntegerLiteralNode*>(e)) {
        indent(out, level);
        out << "IntLiteral: " << il->value << "\n";
        return;
    }

    if (auto* fl = dynamic_cast<FloatLiteralNode*>(e)) {
        indent(out, level);
        out << "FloatLiteral: " << fl->value << "\n";
        return;
    }

    if (auto* bl = dynamic_cast<BoolLiteralNode*>(e)) {
        indent(out, level);
        out << "BoolLiteral: " << (bl->value ? "true" : "false") << "\n";
        return;
    }

    if (auto* cl = dynamic_cast<CharLiteralNode*>(e)) {
        indent(out, level);
        out << "CharLiteral: " << cl->value << "\n";
        return;
    }

    if (auto* sl = dynamic_cast<StringLiteralNode*>(e)) {
        indent(out, level);
        out << "StringLiteral: \"" << sl->value << "\"\n";
        return;
    }

    if (auto* pf = dynamic_cast<PrintfNode*>(e)) {
        indent(out, level);
        out << "Printf\n";
        for (auto arg : pf->args) {
            printExp(out, arg, level + 1);
        }
        return;
    }

    if (auto* tn = dynamic_cast<PrimitiveTypeNode*>(e)) {
        indent(out, level);
        out << "Type: " << typeNodeName(tn) << "\n";
        return;
    }

    if (auto* pt = dynamic_cast<PointerTypeNode*>(e)) {
        indent(out, level);
        out << "PointerType\n";
        printExp(out, pt->base, level + 1);
        return;
    }

    if (auto* st = dynamic_cast<StructTypeNode*>(e)) {
        indent(out, level);
        out << "StructType: " << st->name << "\n";
        return;
    }

    if (auto* nt = dynamic_cast<NamedTypeNode*>(e)) {
        indent(out, level);
        out << "NamedType: " << nt->name << "\n";
        return;
    }

    if (auto* tt = dynamic_cast<TemplateTypeNode*>(e)) {
        indent(out, level);
        out << "TemplateType: " << tt->name << "\n";
        return;
    }

    indent(out, level);
    out << "UnknownExp\n";
}

static void printVarDecl(ostream& out, VarDecl* vd, int level) {
    indent(out, level);
    out << "VarDecl: " << typeNodeName(vd->type) << " " << vd->name;
    for ([[maybe_unused]] auto sz : vd->array_sizes) {
        out << "[]";
    }
    out << "\n";
    if (vd->initializer) {
        indent(out, level + 1);
        out << "Initializer\n";
        printExp(out, vd->initializer, level + 2);
    }
}

static void printStm(ostream& out, Stm* s, int level) {
    if (!s) return;

    if (auto* body = dynamic_cast<Body*>(s)) {
        indent(out, level);
        out << "Block\n";
        for (auto st : body->stmts) {
            printStm(out, st, level + 1);
        }
        return;
    }

    if (auto* es = dynamic_cast<ExprStmtNode*>(s)) {
        indent(out, level);
        out << "ExprStmt\n";
        if (es->expr) printExp(out, es->expr, level + 1);
        return;
    }

    if (auto* ifs = dynamic_cast<IfStmt*>(s)) {
        indent(out, level);
        out << "If\n";
        printExp(out, ifs->condition, level + 1);
        printStm(out, ifs->then_branch, level + 1);
        if (ifs->else_branch) {
            indent(out, level + 1);
            out << "Else\n";
            printStm(out, ifs->else_branch, level + 2);
        }
        return;
    }

    if (auto* wh = dynamic_cast<WhileStmt*>(s)) {
        indent(out, level);
        out << "While\n";
        printExp(out, wh->condition, level + 1);
        printStm(out, wh->body, level + 1);
        return;
    }

    if (auto* dw = dynamic_cast<DoWhileStmt*>(s)) {
        indent(out, level);
        out << "DoWhile\n";
        printStm(out, dw->body, level + 1);
        printExp(out, dw->condition, level + 1);
        return;
    }

    if (auto* fr = dynamic_cast<ForStmt*>(s)) {
        indent(out, level);
        out << "For\n";
        if (fr->init) printStm(out, fr->init, level + 1);
        else { indent(out, level + 1); out << "(none)\n"; }
        if (fr->condition) printExp(out, fr->condition, level + 1);
        else { indent(out, level + 1); out << "(none)\n"; }
        if (fr->increment) printExp(out, fr->increment, level + 1);
        else { indent(out, level + 1); out << "(none)\n"; }
        printStm(out, fr->body, level + 1);
        return;
    }

    if (auto* sw = dynamic_cast<SwitchStmt*>(s)) {
        indent(out, level);
        out << "Switch\n";
        printExp(out, sw->expr, level + 1);
        for (auto cc : sw->cases) {
            indent(out, level + 1);
            out << "Case\n";
            printExp(out, cc->value, level + 2);
            for (auto st : cc->body) {
                printStm(out, st, level + 2);
            }
        }
        if (!sw->default_body.empty()) {
            indent(out, level + 1);
            out << "Default\n";
            for (auto st : sw->default_body) {
                printStm(out, st, level + 2);
            }
        }
        return;
    }

    if (dynamic_cast<BreakStmt*>(s)) {
        indent(out, level);
        out << "Break\n";
        return;
    }

    if (dynamic_cast<ContinueStmt*>(s)) {
        indent(out, level);
        out << "Continue\n";
        return;
    }

    if (auto* ret = dynamic_cast<ReturnStmt*>(s)) {
        indent(out, level);
        out << "Return\n";
        if (ret->expr) printExp(out, ret->expr, level + 1);
        return;
    }

    if (auto* fr = dynamic_cast<FreeStmt*>(s)) {
        indent(out, level);
        out << "Free\n";
        printExp(out, fr->expr, level + 1);
        return;
    }

    if (auto* vd = dynamic_cast<VarDecl*>(s)) {
        printVarDecl(out, vd, level);
        return;
    }

    indent(out, level);
    out << "UnknownStm\n";
}

void printAST(Program* program, ostream& out) {
    out << "Program\n";

    for (auto s : program->structs) {
        indent(out, 1);
        out << "StructDecl: " << s->name << "\n";
        for (auto m : s->members) {
            printVarDecl(out, m, 2);
        }
    }

    for (auto t : program->templates) {
        indent(out, 1);
        out << "TemplateDecl";
        for (size_t i = 0; i < t->params.size(); i++) {
            out << (i == 0 ? ": " : ", ") << t->params[i];
        }
        out << "\n";
        if (t->func) {
            // Print the template function pattern
        }
        if (t->struct_decl) {
            // Print the template struct pattern
        }
    }

    for (auto g : program->globals) {
        printVarDecl(out, g, 1);
    }

    for (auto f : program->functions) {
        indent(out, 1);
        out << "FunctionDecl: " << f->name << "\n";
        for (auto p : f->params) {
            printVarDecl(out, p, 2);
        }
        printStm(out, f->body, 2);
    }
}
