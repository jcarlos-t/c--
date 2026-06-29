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
    last_string = e->value;
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
    // Handle struct member assignment: obj.member = expr
    if (auto* ma = dynamic_cast<MemberAccessNode*>(e->target)) {
        if (auto* id = dynamic_cast<IdentifierNode*>(ma->object)) {
            double val = e->value->accept(this);
            auto it = struct_instances.find(id->name);
            if (it != struct_instances.end()) {
                it->second[ma->member] = val;
                return val;
            }
        }
        return 0;
    }
    // Handle array element assignment: arr[idx] = expr
    if (auto* sub = dynamic_cast<IndexNode*>(e->target)) {
        if (auto* id = dynamic_cast<IdentifierNode*>(sub->base)) {
            int idx = (int)sub->index->accept(this);
            double val = e->value->accept(this);
            auto it = array_data.find(id->name);
            if (it == array_data.end() || idx < 0 || (size_t)idx >= it->second.size()) {
                cerr << "Error: índice de arreglo fuera de rango" << endl;
                return 0;
            }
            switch (e->op) {
                case AssignOp::ASSIGN: it->second[idx] = val; return val;
            }
        }
        return 0;
    }
    if (auto* id = dynamic_cast<IdentifierNode*>(e->target)) {
        double val = e->value->accept(this);
        switch (e->op) {
            case AssignOp::ASSIGN:
                env.update(id->name, val);
                return val;
        }
    }
    return 0;
}

double EVALVisitor::visit(PrintfNode* e) {
    for (auto a : e->args) {
        if (dynamic_cast<StringLiteralNode*>(a)) {
            a->accept(this);
            cout << last_string;
        } else {
            double val = a->accept(this);
            string type = getType(a);
            if (type == "bool")
                cout << (val != 0 ? "true" : "false");
            else
                cout << val;
        }
    }
    cout << endl;
    return 0;
}

double EVALVisitor::visit(FcallNode* e) {
    if (auto* id = dynamic_cast<IdentifierNode*>(e->callee)) {
        string fname = id->name;
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

double EVALVisitor::visit(MallocNode* e) {
    int size = (int)e->size->accept(this);
    int addr = next_addr++;
    heap[addr] = vector<double>(size, 0);
    return (double)addr;
}

double EVALVisitor::visit(IndexNode* e) {
    if (auto* id = dynamic_cast<IdentifierNode*>(e->base)) {
        int idx = (int)e->index->accept(this);
        auto it = array_data.find(id->name);
        if (it != array_data.end() && idx >= 0 && (size_t)idx < it->second.size())
            return it->second[idx];
        cerr << "Error: índice de arreglo fuera de rango" << endl;
        return 0;
    }
    cerr << "Error: subíndices no soportados en evaluación" << endl;
    return 0;
}

double EVALVisitor::visit(MemberAccessNode* e) {
    if (auto* id = dynamic_cast<IdentifierNode*>(e->object)) {
        auto it = struct_instances.find(id->name);
        if (it != struct_instances.end()) {
            auto mit = it->second.find(e->member);
            if (mit != it->second.end()) return mit->second;
        }
    }
    cerr << "Error: acceso a miembros no soportado en evaluación" << endl;
    return 0;
}

double EVALVisitor::visit(ArrowAccessNode* e) {
    if (auto* id = dynamic_cast<IdentifierNode*>(e->pointer)) {
        // Arrow dereferences a pointer: ptr->member
        // For simplicity, treat it like dot access on the named variable
        auto it = struct_instances.find(id->name);
        if (it != struct_instances.end()) {
            auto mit = it->second.find(e->member);
            if (mit != it->second.end()) return mit->second;
        }
    }
    cerr << "Error: acceso por flecha no soportado en evaluación" << endl;
    return 0;
}

double EVALVisitor::visit(SizeOfNode* e) {
    if (auto* pt = dynamic_cast<PrimitiveTypeNode*>(e->target_type)) {
        switch (pt->prim) {
            case PrimitiveTypeNode::VOID: return 1;
            case PrimitiveTypeNode::CHAR: return 1;
            case PrimitiveTypeNode::BOOL: return 1;
            case PrimitiveTypeNode::INT: return 4;
            case PrimitiveTypeNode::FLOAT: return 4;
            case PrimitiveTypeNode::DOUBLE: return 8;
            case PrimitiveTypeNode::AUTO: return 0;
        }
    }
    if (dynamic_cast<PointerTypeNode*>(e->target_type)) return 8;
    return 0;
}

double EVALVisitor::visit(PrimitiveTypeNode* e) { return 0; }
double EVALVisitor::visit(PointerTypeNode* e) { return 0; }
double EVALVisitor::visit(StructTypeNode* e) { return 0; }
double EVALVisitor::visit(NamedTypeNode* e) { return 0; }

double EVALVisitor::visit(TemplateTypeNode* e) { return 0; }

int EVALVisitor::visit(Body* s) {
    env.add_level();
    typeEnv.add_level();
    try {
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
    env.add_level();
    typeEnv.add_level();
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
    typeEnv.remove_level();
    env.remove_level();
    return 0;
}

int EVALVisitor::visit(SwitchStmt* s) {
    double val = s->expr->accept(this);
    bool matched = false;
    for (auto cc : s->cases) {
        double case_val = cc->value->accept(this);
        if (matched || val == case_val) {
            matched = true;
            for (auto st : cc->body) {
                try { st->accept(this); } catch (BreakException&) { return 0; }
            }
        }
    }
    if (!matched) {
        for (auto st : s->default_body) {
            try { st->accept(this); } catch (BreakException&) { return 0; }
        }
    }
    return 0;
}

int EVALVisitor::visit(CaseClause* s) { return 0; }
int EVALVisitor::visit(BreakStmt* s) { throw BreakException(); }
int EVALVisitor::visit(ContinueStmt* s) { throw ContinueException(); }

int EVALVisitor::visit(ReturnStmt* s) {
    if (s->expr) throw ReturnException(s->expr->accept(this));
    throw ReturnException(0);
}

int EVALVisitor::visit(FreeStmt* s) {
    double ptr = s->expr->accept(this);
    int addr = (int)ptr;
    heap.erase(addr);
    return 0;
}

int EVALVisitor::visit(VarDecl* d) {
    env.add_var(d->name);
    typeEnv.add_var(d->name, "");
    if (auto* st = dynamic_cast<StructTypeNode*>(d->type)) {
        unordered_map<string, double> members;
        auto it = struct_defs.find(st->name);
        if (it != struct_defs.end()) {
            for (auto& m : it->second) members[m] = 0;
        }
        struct_instances[d->name] = members;
        env.update(d->name, 0);
    } else if (!d->array_sizes.empty()) {
        int size = 1;
        for (auto s : d->array_sizes) size *= (int)s->accept(this);
        array_data[d->name] = vector<double>(size, 0);
    } else if (d->initializer) {
        env.update(d->name, d->initializer->accept(this));
    }
    return 0;
}

int EVALVisitor::visit(FunDecl* d) {
    envfun[d->name] = d;
    funReturnTypes[d->name] = "";
    return 0;
}

int EVALVisitor::visit(StructDecl* d) {
    vector<string> members;
    for (auto m : d->members) members.push_back(m->name);
    struct_defs[d->name] = members;
    return 0;
}

double EVALVisitor::visit(LambdaExprNode* e) { return 0; }

double EVALVisitor::visit(CaptureNode* e) { return 0; }

int EVALVisitor::visit(TemplateDecl* d) { return 0; }

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
    if (dynamic_cast<StringLiteralNode*>(e)) return "string";
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
