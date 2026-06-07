#include <iostream>
#include <cmath>
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
double TernaryOpNode::accept(Visitor* v) { return v->visit(this); }
double CallNode::accept(Visitor* v) { return v->visit(this); }
double SubscriptNode::accept(Visitor* v) { return v->visit(this); }
double MemberAccessNode::accept(Visitor* v) { return v->visit(this); }
double ArrowAccessNode::accept(Visitor* v) { return v->visit(this); }
double CastNode::accept(Visitor* v) { return v->visit(this); }
double IdentifierNode::accept(Visitor* v) { return v->visit(this); }
double IntegerLiteralNode::accept(Visitor* v) { return v->visit(this); }
double FloatLiteralNode::accept(Visitor* v) { return v->visit(this); }
double BoolLiteralNode::accept(Visitor* v) { return v->visit(this); }
double CharLiteralNode::accept(Visitor* v) { return v->visit(this); }
double StringLiteralNode::accept(Visitor* v) { return v->visit(this); }
double ParenthesizedExprNode::accept(Visitor* v) { return v->visit(this); }
double PrimitiveTypeNode::accept(Visitor* v) { return v->visit(this); }
double PointerTypeNode::accept(Visitor* v) { return v->visit(this); }
double StructTypeNode::accept(Visitor* v) { return v->visit(this); }
double NamedTypeNode::accept(Visitor* v) { return v->visit(this); }

// ============================================================
// accept() methods for Visitor (int-returning)
// Statements & Declarations
// ============================================================
int CompoundStmt::accept(Visitor* v) { return v->visit(this); }
int ExprStmtNode::accept(Visitor* v) { return v->visit(this); }
int IfStmt::accept(Visitor* v) { return v->visit(this); }
int WhileStmt::accept(Visitor* v) { return v->visit(this); }
int DoWhileStmt::accept(Visitor* v) { return v->visit(this); }
int ForStmt::accept(Visitor* v) { return v->visit(this); }
int SwitchStmt::accept(Visitor* v) { return v->visit(this); }
int BreakStmt::accept(Visitor* v) { return v->visit(this); }
int ContinueStmt::accept(Visitor* v) { return v->visit(this); }
int ReturnStmt::accept(Visitor* v) { return v->visit(this); }
int VarDecl::accept(Visitor* v) { return v->visit(this); }
int FunDecl::accept(Visitor* v) { return v->visit(this); }
int StructDecl::accept(Visitor* v) { return v->visit(this); }
int Program::accept(Visitor* v) { return v->visit(this); }

// ============================================================
// PrintVisitor
// ============================================================
void PrintVisitor::print_indent() {
    for (int i = 0; i < indent; i++) cout << "  ";
}

double PrintVisitor::visit(BinaryOpNode* e) {
    cout << "(";
    e->left->accept(this);
    switch (e->op) {
        case BinaryOp::ADD: cout << " + "; break;
        case BinaryOp::SUB: cout << " - "; break;
        case BinaryOp::MUL: cout << " * "; break;
        case BinaryOp::DIV: cout << " / "; break;
        case BinaryOp::MOD: cout << " % "; break;
        case BinaryOp::EQ: cout << " == "; break;
        case BinaryOp::NE: cout << " != "; break;
        case BinaryOp::LT: cout << " < "; break;
        case BinaryOp::GT: cout << " > "; break;
        case BinaryOp::LE: cout << " <= "; break;
        case BinaryOp::GE: cout << " >= "; break;
        case BinaryOp::LOG_AND: cout << " && "; break;
        case BinaryOp::LOG_OR: cout << " || "; break;
        case BinaryOp::POW: cout << " ** "; break;
        case BinaryOp::COMMA: cout << " , "; break;
    }
    e->right->accept(this);
    cout << ")";
    return 0;
}

double PrintVisitor::visit(UnaryOpNode* e) {
    switch (e->op) {
        case UnaryOp::PRE_INC: cout << "++"; break;
        case UnaryOp::PRE_DEC: cout << "--"; break;
        case UnaryOp::ADDR: cout << "&"; break;
        case UnaryOp::DEREF: cout << "*"; break;
        case UnaryOp::MINUS: cout << "-"; break;
        case UnaryOp::LOG_NOT: cout << "!"; break;
        default: break;
    }
    e->operand->accept(this);
    if (e->op == UnaryOp::POST_INC) cout << "++";
    if (e->op == UnaryOp::POST_DEC) cout << "--";
    return 0;
}

double PrintVisitor::visit(AssignmentNode* e) {
    e->target->accept(this);
    switch (e->op) {
        case AssignOp::ASSIGN: cout << " = "; break;
        case AssignOp::ADD_ASSIGN: cout << " += "; break;
        case AssignOp::SUB_ASSIGN: cout << " -= "; break;
        case AssignOp::MUL_ASSIGN: cout << " *= "; break;
        case AssignOp::DIV_ASSIGN: cout << " /= "; break;
    }
    e->value->accept(this);
    return 0;
}

double PrintVisitor::visit(TernaryOpNode* e) {
    e->condition->accept(this);
    cout << " ? ";
    e->then_expr->accept(this);
    cout << " : ";
    e->else_expr->accept(this);
    return 0;
}

double PrintVisitor::visit(CallNode* e) {
    e->callee->accept(this);
    cout << "(";
    for (size_t i = 0; i < e->args.size(); i++) {
        if (i > 0) cout << ", ";
        e->args[i]->accept(this);
    }
    cout << ")";
    return 0;
}

double PrintVisitor::visit(SubscriptNode* e) {
    e->base->accept(this);
    cout << "[";
    e->index->accept(this);
    cout << "]";
    return 0;
}

double PrintVisitor::visit(MemberAccessNode* e) {
    e->object->accept(this);
    cout << "." << e->member;
    return 0;
}

double PrintVisitor::visit(ArrowAccessNode* e) {
    e->pointer->accept(this);
    cout << "->" << e->member;
    return 0;
}

double PrintVisitor::visit(CastNode* e) {
    cout << "(";
    e->target_type->accept(this);
    cout << ")";
    e->expr->accept(this);
    return 0;
}

double PrintVisitor::visit(IdentifierNode* e) {
    cout << e->name;
    return 0;
}

double PrintVisitor::visit(IntegerLiteralNode* e) {
    cout << e->value;
    return 0;
}

double PrintVisitor::visit(FloatLiteralNode* e) {
    cout << e->value;
    return 0;
}

double PrintVisitor::visit(BoolLiteralNode* e) {
    cout << (e->value ? "true" : "false");
    return 0;
}

double PrintVisitor::visit(CharLiteralNode* e) {
    cout << "'" << e->value << "'";
    return 0;
}

double PrintVisitor::visit(StringLiteralNode* e) {
    cout << "\"" << e->value << "\"";
    return 0;
}

double PrintVisitor::visit(ParenthesizedExprNode* e) {
    cout << "(";
    e->expr->accept(this);
    cout << ")";
    return 0;
}

double PrintVisitor::visit(PrimitiveTypeNode* e) {
    switch (e->prim) {
        case PrimitiveTypeNode::VOID: cout << "void"; break;
        case PrimitiveTypeNode::INT: cout << "int"; break;
        case PrimitiveTypeNode::CHAR: cout << "char"; break;
        case PrimitiveTypeNode::FLOAT: cout << "float"; break;
        case PrimitiveTypeNode::DOUBLE: cout << "double"; break;
        case PrimitiveTypeNode::AUTO: cout << "auto"; break;
    }
    return 0;
}

double PrintVisitor::visit(PointerTypeNode* e) {
    e->base->accept(this);
    cout << "*";
    return 0;
}

double PrintVisitor::visit(StructTypeNode* e) {
    cout << "struct " << e->name;
    return 0;
}

double PrintVisitor::visit(NamedTypeNode* e) {
    cout << e->name;
    return 0;
}

int PrintVisitor::visit(CompoundStmt* s) {
    cout << "{\n";
    indent++;
    for (auto v : s->vdlist) v->accept(this);
    for (auto st : s->stmts) {
        st->accept(this);
    }
    indent--;
    print_indent();
    cout << "}\n";
    return 0;
}

int PrintVisitor::visit(ExprStmtNode* s) {
    print_indent();
    if (s->expr) {
        s->expr->accept(this);
    }
    cout << ";\n";
    return 0;
}

int PrintVisitor::visit(IfStmt* s) {
    print_indent();
    cout << "if (";
    s->condition->accept(this);
    cout << ")\n";
    s->then_branch->accept(this);
    if (s->else_branch) {
        print_indent();
        cout << "else\n";
        s->else_branch->accept(this);
    }
    return 0;
}

int PrintVisitor::visit(WhileStmt* s) {
    print_indent();
    cout << "while (";
    s->condition->accept(this);
    cout << ")\n";
    s->body->accept(this);
    return 0;
}

int PrintVisitor::visit(DoWhileStmt* s) {
    print_indent();
    cout << "do\n";
    s->body->accept(this);
    print_indent();
    cout << "while (";
    s->condition->accept(this);
    cout << ");\n";
    return 0;
}

int PrintVisitor::visit(ForStmt* s) {
    print_indent();
    cout << "for (";
    if (s->init) s->init->accept(this);
    else cout << "; ";
    if (s->condition) { s->condition->accept(this); cout << "; "; }
    else cout << "; ";
    if (s->increment) s->increment->accept(this);
    cout << ")\n";
    s->body->accept(this);
    return 0;
}

int PrintVisitor::visit(SwitchStmt* s) {
    print_indent();
    cout << "switch (";
    s->expr->accept(this);
    cout << ") { ... }\n";
    return 0;
}

int PrintVisitor::visit(BreakStmt* s) {
    print_indent();
    cout << "break;\n";
    return 0;
}

int PrintVisitor::visit(ContinueStmt* s) {
    print_indent();
    cout << "continue;\n";
    return 0;
}

int PrintVisitor::visit(ReturnStmt* s) {
    print_indent();
    cout << "return";
    if (s->expr) {
        cout << " ";
        s->expr->accept(this);
    }
    cout << ";\n";
    return 0;
}

int PrintVisitor::visit(VarDecl* d) {
    print_indent();
    d->type->accept(this);
    cout << " " << d->name;
    for (auto s : d->array_sizes) {
        cout << "[";
        s->accept(this);
        cout << "]";
    }
    if (d->initializer) {
        cout << " = ";
        d->initializer->accept(this);
    }
    cout << ";\n";
    return 0;
}

int PrintVisitor::visit(FunDecl* d) {
    print_indent();
    d->return_type->accept(this);
    cout << " " << d->name << "(";
    for (size_t i = 0; i < d->params.size(); i++) {
        if (i > 0) cout << ", ";
        d->params[i]->type->accept(this);
        cout << " " << d->params[i]->name;
    }
    cout << ")\n";
    d->body->accept(this);
    cout << "\n";
    return 0;
}

int PrintVisitor::visit(StructDecl* d) {
    print_indent();
    cout << "struct " << d->name << " {\n";
    indent++;
    for (auto m : d->members) {
        m->accept(this);
    }
    indent--;
    print_indent();
    cout << "};\n";
    return 0;
}

int PrintVisitor::visit(Program* p) {
    for (auto s : p->structs) s->accept(this);
    for (auto g : p->globals) g->accept(this);
    for (auto f : p->functions) f->accept(this);
    return 0;
}

void PrintVisitor::imprimir(Program* programa) {
    if (programa) {
        cout << "Program:\n";
        programa->accept(this);
    }
}

// ============================================================
// EVALVisitor
// ============================================================

double EVALVisitor::visit(IntegerLiteralNode* e) {
    return (double)e->value;
}

double EVALVisitor::visit(FloatLiteralNode* e) {
    return e->value;
}

double EVALVisitor::visit(BoolLiteralNode* e) {
    return e->value ? 1.0 : 0.0;
}

double EVALVisitor::visit(CharLiteralNode* e) {
    return (double)e->value;
}

double EVALVisitor::visit(StringLiteralNode* e) {
    cerr << "Error: string literales no soportados en evaluación" << endl;
    return 0;
}

double EVALVisitor::visit(IdentifierNode* e) {
    return env.lookup(e->name);
}

double EVALVisitor::visit(BinaryOpNode* e) {
    double v1 = e->left->accept(this);
    double v2 = e->right->accept(this);
    switch (e->op) {
        case BinaryOp::ADD: return v1 + v2;
        case BinaryOp::SUB: return v1 - v2;
        case BinaryOp::MUL: return v1 * v2;
        case BinaryOp::DIV:
            if (v2 != 0) return v1 / v2;
            cerr << "Error: división por cero" << endl; return 0;
        case BinaryOp::MOD:
            if (v2 != 0) return (long long)v1 % (long long)v2;
            cerr << "Error: módulo por cero" << endl; return 0;
        case BinaryOp::POW: return pow(v1, v2);
        case BinaryOp::EQ: return (v1 == v2) ? 1.0 : 0.0;
        case BinaryOp::NE: return (v1 != v2) ? 1.0 : 0.0;
        case BinaryOp::LT: return (v1 < v2) ? 1.0 : 0.0;
        case BinaryOp::GT: return (v1 > v2) ? 1.0 : 0.0;
        case BinaryOp::LE: return (v1 <= v2) ? 1.0 : 0.0;
        case BinaryOp::GE: return (v1 >= v2) ? 1.0 : 0.0;
        case BinaryOp::LOG_AND: return (v1 != 0 && v2 != 0) ? 1.0 : 0.0;
        case BinaryOp::LOG_OR: return (v1 != 0 || v2 != 0) ? 1.0 : 0.0;
        default: return 0;
    }
}

double EVALVisitor::visit(UnaryOpNode* e) {
    double v = e->operand->accept(this);
    switch (e->op) {
        case UnaryOp::PRE_INC: {
            if (auto* id = dynamic_cast<IdentifierNode*>(e->operand)) {
                v = v + 1.0;
                env.update(id->name, v);
                return v;
            }
            return v + 1.0;
        }
        case UnaryOp::PRE_DEC: {
            if (auto* id = dynamic_cast<IdentifierNode*>(e->operand)) {
                v = v - 1.0;
                env.update(id->name, v);
                return v;
            }
            return v - 1.0;
        }
        case UnaryOp::POST_INC: {
            if (auto* id = dynamic_cast<IdentifierNode*>(e->operand)) {
                env.update(id->name, v + 1.0);
                return v;
            }
            return v;
        }
        case UnaryOp::POST_DEC: {
            if (auto* id = dynamic_cast<IdentifierNode*>(e->operand)) {
                env.update(id->name, v - 1.0);
                return v;
            }
            return v;
        }
        case UnaryOp::MINUS: return -v;
        case UnaryOp::LOG_NOT: return (v == 0) ? 1.0 : 0.0;
        default: return v;
    }
}

double EVALVisitor::visit(AssignmentNode* e) {
    if (auto* id = dynamic_cast<IdentifierNode*>(e->target)) {
        double val = e->value->accept(this);
        switch (e->op) {
            case AssignOp::ASSIGN:
                env.update(id->name, val);
                return val;
            case AssignOp::ADD_ASSIGN: {
                double cur = env.lookup(id->name);
                double res = cur + val;
                env.update(id->name, res);
                return res;
            }
            case AssignOp::SUB_ASSIGN: {
                double cur = env.lookup(id->name);
                double res = cur - val;
                env.update(id->name, res);
                return res;
            }
            case AssignOp::MUL_ASSIGN: {
                double cur = env.lookup(id->name);
                double res = cur * val;
                env.update(id->name, res);
                return res;
            }
            case AssignOp::DIV_ASSIGN: {
                double cur = env.lookup(id->name);
                if (val == 0) { cerr << "Error: división por cero" << endl; return 0; }
                double res = cur / val;
                env.update(id->name, res);
                return res;
            }
        }
    }
    return 0;
}

double EVALVisitor::visit(TernaryOpNode* e) {
    double cond = e->condition->accept(this);
    if (cond != 0) return e->then_expr->accept(this);
    return e->else_expr->accept(this);
}

double EVALVisitor::visit(CallNode* e) {
    if (auto* id = dynamic_cast<IdentifierNode*>(e->callee)) {
        string fname = id->name;
        if (fname == "print" || fname == "printf") {
            for (auto a : e->args) {
                double val = a->accept(this);
                string type = getType(a);
                if (type == "bool")
                    cout << (val != 0 ? "true" : "false");
                else
                    cout << val;
            }
            cout << endl;
            return 0;
        }

        vector<double> arg;
        for (auto a : e->args) arg.push_back(a->accept(this));

        FunDecl* fd = envfun[fname];
        if (!fd) {
            cerr << "Error: función no declarada '" << fname << "'" << endl;
            exit(0);
        }

        env.add_level();
        typeEnv.add_level();
        for (size_t i = 0; i < arg.size() && i < fd->params.size(); i++) {
            env.add_var(fd->params[i]->name, arg[i]);
            typeEnv.add_var(fd->params[i]->name, "");
        }
        double result = 0;
        try {
            fd->body->accept(this);
        } catch (ReturnException& re) {
            result = re.value;
        } catch (...) {
            typeEnv.remove_level();
            env.remove_level();
            throw;
        }
        typeEnv.remove_level();
        env.remove_level();
        return result;
    }
    cerr << "Error: llamada a no identificador no soportada" << endl;
    return 0;
}

double EVALVisitor::visit(SubscriptNode* e) {
    cerr << "Error: subíndices no soportados en evaluación" << endl;
    return 0;
}

double EVALVisitor::visit(MemberAccessNode* e) {
    cerr << "Error: acceso a miembros no soportado en evaluación" << endl;
    return 0;
}

double EVALVisitor::visit(ArrowAccessNode* e) {
    cerr << "Error: acceso por flecha no soportado en evaluación" << endl;
    return 0;
}

double EVALVisitor::visit(CastNode* e) {
    double val = e->expr->accept(this);
    if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(e->target_type)) {
        if (pt->prim == PrimitiveTypeNode::INT) return (int)val;
        if (pt->prim == PrimitiveTypeNode::FLOAT) return (float)val;
        if (pt->prim == PrimitiveTypeNode::DOUBLE) return (double)val;
        if (pt->prim == PrimitiveTypeNode::CHAR) return (char)val;
        if (pt->prim == PrimitiveTypeNode::BOOL) return val != 0 ? 1.0 : 0.0;
    }
    return val;
}

double EVALVisitor::visit(ParenthesizedExprNode* e) {
    return e->expr->accept(this);
}

double EVALVisitor::visit(PrimitiveTypeNode* e) { return 0; }
double EVALVisitor::visit(PointerTypeNode* e) { return 0; }
double EVALVisitor::visit(StructTypeNode* e) { return 0; }
double EVALVisitor::visit(NamedTypeNode* e) { return 0; }

int EVALVisitor::visit(CompoundStmt* s) {
    env.add_level();
    typeEnv.add_level();
    try {
        for (auto v : s->vdlist) v->accept(this);
        for (auto st : s->stmts) st->accept(this);
    } catch (...) {
        typeEnv.remove_level();
        env.remove_level();
        throw;
    }
    typeEnv.remove_level();
    env.remove_level();
    return 0;
}

int EVALVisitor::visit(ExprStmtNode* s) {
    if (s->expr) s->expr->accept(this);
    return 0;
}

int EVALVisitor::visit(IfStmt* s) {
    double cond = s->condition->accept(this);
    if (cond != 0) s->then_branch->accept(this);
    else if (s->else_branch) s->else_branch->accept(this);
    return 0;
}

int EVALVisitor::visit(WhileStmt* s) {
    while (true) {
        double cond = s->condition->accept(this);
        if (cond == 0) break;
        try {
            s->body->accept(this);
        } catch (BreakException&) {
            break;
        } catch (ContinueException&) {
            continue;
        }
    }
    return 0;
}

int EVALVisitor::visit(DoWhileStmt* s) {
    do {
        try {
            s->body->accept(this);
        } catch (BreakException&) {
            break;
        } catch (ContinueException&) {
            continue;
        }
    } while (s->condition->accept(this) != 0);
    return 0;
}

int EVALVisitor::visit(ForStmt* s) {
    if (s->init) s->init->accept(this);
    while (true) {
        if (s->condition && s->condition->accept(this) == 0) break;
        try {
            s->body->accept(this);
        } catch (BreakException&) {
            break;
        } catch (ContinueException&) {
            if (s->increment) s->increment->accept(this);
            continue;
        }
        if (s->increment) s->increment->accept(this);
    }
    return 0;
}

int EVALVisitor::visit(SwitchStmt* s) {
    cerr << "Error: switch no soportado en evaluación" << endl;
    return 0;
}

int EVALVisitor::visit(BreakStmt* s) { throw BreakException(); }
int EVALVisitor::visit(ContinueStmt* s) { throw ContinueException(); }

int EVALVisitor::visit(ReturnStmt* s) {
    if (s->expr) throw ReturnException(s->expr->accept(this));
    throw ReturnException(0);
}

int EVALVisitor::visit(VarDecl* d) {
    for (auto& a : d->array_sizes) (void)a;
    env.add_var(d->name);
    typeEnv.add_var(d->name, "");
    if (d->initializer) {
        env.update(d->name, d->initializer->accept(this));
    }
    return 0;
}

int EVALVisitor::visit(FunDecl* d) {
    envfun[d->name] = d;
    funReturnTypes[d->name] = "";
    return 0;
}

int EVALVisitor::visit(StructDecl* d) { return 0; }

int EVALVisitor::visit(Program* p) {
    env.add_level();
    typeEnv.add_level();
    for (auto g : p->globals) g->accept(this);
    for (auto f : p->functions) f->accept(this);
    if (envfun.count("main")) {
        try {
            envfun["main"]->body->accept(this);
        } catch (ReturnException&) {
        }
    } else {
        cerr << "Error: no existe función main" << endl;
        exit(0);
    }
    typeEnv.remove_level();
    env.remove_level();
    return 0;
}

string EVALVisitor::getType(Exp* e) {
    if (dynamic_cast<BoolLiteralNode*>(e)) return "bool";
    if (dynamic_cast<IntegerLiteralNode*>(e)) return "int";
    if (dynamic_cast<FloatLiteralNode*>(e)) return "float";
    if (dynamic_cast<CharLiteralNode*>(e)) return "char";
    if (auto* id = dynamic_cast<IdentifierNode*>(e)) return typeEnv.lookup(id->name);
    if (auto* be = dynamic_cast<BinaryOpNode*>(e)) {
        if (be->op == BinaryOp::EQ || be->op == BinaryOp::NE ||
            be->op == BinaryOp::LT || be->op == BinaryOp::GT ||
            be->op == BinaryOp::LE || be->op == BinaryOp::GE ||
            be->op == BinaryOp::LOG_AND || be->op == BinaryOp::LOG_OR)
            return "bool";
        string lt = getType(be->left);
        string rt = getType(be->right);
        return (lt == "float" || rt == "float") ? "float" : "int";
    }
    return "int";
}

void EVALVisitor::interprete(Program* programa) {
    if (programa) {
        cout << "Interprete:" << endl;
        programa->accept(this);
    }
}
