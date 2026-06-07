#include <iostream>
#include <fstream>
#include <cmath>
#include "ast.h"
#include "visitor.h"

using namespace std;


///////////////////////////////////////////////////////////////////////////////////
//                    SECCIÓN 1: MÉTODOS accept() DEL AST
///////////////////////////////////////////////////////////////////////////////////

// Expresiones
double BinaryExp::accept(Visitor* visitor) { return visitor->visit(this); }
double BoolExp::accept(Visitor* visitor) { return visitor->visit(this); }
double NumberExp::accept(Visitor* visitor) { return visitor->visit(this); }
double IdExp::accept(Visitor* visitor)     { return visitor->visit(this); }
double FcallExp::accept(Visitor* visitor)  { return visitor->visit(this); }

// Sentencias
int PrintStm::accept(Visitor* visitor)  { return visitor->visit(this); }
int AssignStm::accept(Visitor* visitor) { return visitor->visit(this); }
int ReturnStm::accept(Visitor* visitor) { return visitor->visit(this); }
int WhileStm::accept(Visitor* visitor) { return visitor->visit(this); }
int IfStm::accept(Visitor* visitor) { return visitor->visit(this); }

// Declaraciones
int VarDec::accept(Visitor* visitor)    { return visitor->visit(this); }
int FunDec::accept(Visitor* visitor)    { return visitor->visit(this); }

// Estructuras compuestas
int Body::accept(Visitor* visitor)      { return visitor->visit(this); }
int Program::accept(Visitor* visitor)   { return visitor->visit(this); }

///////////////////////////////////////////////////////////////////////////////////
//                    SECCIÓN 2: IMPLEMENTACIÓN DE PrintVisitor
///////////////////////////////////////////////////////////////////////////////////

double PrintVisitor::visit(NumberExp* exp) {
    cout << exp->value;
    return 0;
}

double PrintVisitor::visit(IdExp* exp) {
    cout << exp->value;
    return 0;
}

double PrintVisitor::visit(BoolExp* exp) {
    if (exp->value==1){
        cout << "true";
    }
    else{
        cout << "false";
    }
    return 0;
}

double PrintVisitor::visit(BinaryExp* exp) {
    exp->left->accept(this);
    cout << ' ' << Exp::binopToChar(exp->op) << ' ';
    exp->right->accept(this);
    return 0;
}

int PrintVisitor::visit(PrintStm* stm) {
    cout << "print(";
    stm->e->accept(this);
    cout << ")" << endl;
    return 0;
}

int PrintVisitor::visit(AssignStm* stm) {
    cout << stm->id << " = ";
    stm->e->accept(this);
    cout << endl;
    return 0;
}

int PrintVisitor::visit(ReturnStm* stm) {
    cout << "return ";
    if (stm->e) stm->e->accept(this);
    cout << endl;
    return 0;
}

int PrintVisitor::visit(WhileStm* stm) {
    cout << "while (";
    stm->condition->accept(this);
    cout << ") ";
    stm->body->accept(this);
    cout << "endwhile" << endl;
    return 0;
}

int PrintVisitor::visit(IfStm* stm) {
    cout << "if (";
    stm->condition->accept(this);
    cout << ") ";
    stm->thenBody->accept(this);
    if (stm->elseBody) {
        cout << "else ";
        stm->elseBody->accept(this);
    }
    cout << "endif" << endl;
    return 0;
}

int PrintVisitor::visit(VarDec* vd) {
    cout << "var " << vd->tipo;
    for(auto i:vd->variables){
        cout<<" " << i << ",";
    }
    cout << ";" << endl;
    return 0;
}

int PrintVisitor::visit(Body* b) {
    cout << "{\n";
    for (auto i : b->vdlist){
        i->accept(this);
    }
    for (auto i : b->stmlist){
        i->accept(this);
    }
    cout << "}\n";
    return 0;
}

double PrintVisitor::visit(FcallExp* fcall) {
    cout << fcall-> nombre << "(";
    for (auto i : fcall->argumentos) {
        i->accept(this);
        cout << ",";
    }
    cout << ")";
    return 0;
}

int PrintVisitor::visit(FunDec* fd) {
    cout << "fun " << fd->tipo << " " << fd->nombre << "(";
    for (size_t i = 0; i <  fd->Nparametros.size(); ++i) {
        cout << fd->Tparametros[i];
        if (i < fd->Nparametros.size() - 1)
            cout << ", ";
    }
    cout << ") ";
    fd->cuerpo->accept(this);
    cout << endl;
    return 0;
}

int PrintVisitor::visit(Program* p) {
    for (auto d : p->vdlist)
        d->accept(this);
    for (auto d : p->fdlist)
        d->accept(this);
    return 0;
}

void PrintVisitor::imprimir(Program* programa) {
    if (programa) {
        programa->accept(this);
    }
}

///////////////////////////////////////////////////////////////////////////////////
//                    SECCIÓN 3: IMPLEMENTACIÓN DE EVALVisitor
///////////////////////////////////////////////////////////////////////////////////

double EVALVisitor::visit(NumberExp* exp) {
    return exp->value;
}

double EVALVisitor::visit(BoolExp* exp) {
    return exp->value;
}

double EVALVisitor::visit(IdExp* exp) {
    return env.lookup(exp->value);
}

double EVALVisitor::visit(BinaryExp* exp) {
    double v1 = exp->left->accept(this);
    double v2 = exp->right->accept(this);
    double result = 0;

    switch (exp->op) {
        case PLUS_OP:  result = v1 + v2; break;
        case MINUS_OP: result = v1 - v2; break;
        case MUL_OP:   result = v1 * v2; break;
        case DIV_OP:
            if (v2 != 0) result = v1 / v2;
            else {
                cout << "Error: división por cero" << endl;
                result = 0;
            }
            break;
        case POW_OP: result = pow(v1, v2); break;
        case LE_OP: result = v1 < v2; break;
        case AND_OP: result = static_cast<double>(static_cast<bool>(v1) && static_cast<bool>(v2)); break;
        default:
            cout << "Operador desconocido" << endl;
            result = 0;
    }
    return result;
}

int EVALVisitor::visit(PrintStm* p) {
    double val = p->e->accept(this);
    string type = getType(p->e);
    if (type == "bool")
        cout << (val != 0 ? "true" : "false") << endl;
    else
        cout << val << endl;
    return 0;
}

int EVALVisitor::visit(AssignStm* p) {
    env.update(p->id, p->e->accept(this));
    return 0;
}

int EVALVisitor::visit(ReturnStm* stm) {
    retcall = true;
    retval = stm->e->accept(this);
    return 0;
}

int EVALVisitor::visit(WhileStm* stm) {
    while (true) {
        double cond = stm->condition->accept(this);
        if (cond == 0) break;
        stm->body->accept(this);
    }
    return 0;
}

int EVALVisitor::visit(IfStm* stm) {
    double cond = stm->condition->accept(this);
    if (cond != 0) {
        stm->thenBody->accept(this);
    } else if (stm->elseBody) {
        stm->elseBody->accept(this);
    }
    return 0;
}

int EVALVisitor::visit(VarDec* vd) {
    for (auto& i : vd->variables) {
        env.add_var(i);
        typeEnv.add_var(i, vd->tipo);
    }
    return 0;
}

int EVALVisitor::visit(Body* b) {
    env.add_level();
    typeEnv.add_level();
    for (auto i:b->vdlist){
        i->accept(this);
    }
    for (auto i:b->stmlist){
        i->accept(this);
    }
    typeEnv.remove_level();
    env.remove_level();
    return 0;
}

double EVALVisitor::visit(FcallExp* fcall) {
    retcall = false;
    vector<double> arg;
    for (auto i : fcall->argumentos) {
        arg.push_back(i->accept(this));
    } 
    FunDec* fd = envfun[fcall->nombre];
    env.add_level();
    typeEnv.add_level();

    for (size_t i = 0; i < arg.size(); ++i) {
        env.add_var(fd->Nparametros[i], arg[i]);
        typeEnv.add_var(fd->Nparametros[i], fd->Tparametros[i]);
    }

    fd->cuerpo->accept(this);
    typeEnv.remove_level();
    env.remove_level();

    if (fd->tipo == "void") {
        return 0;
    }
    if (retcall) {
        return retval;
    } else {
        cout << "Error: la función '" << fcall->nombre << "' no tiene retorno" << endl;
        exit(0);
    }
}

int EVALVisitor::visit(FunDec* fd) {
    envfun[fd->nombre] = fd;
    funReturnTypes[fd->nombre] = fd->tipo;
    return 0;
}

int EVALVisitor::visit(Program* p) {
    env.add_level();
    typeEnv.add_level();
    for(auto i:p->vdlist){
        i->accept(this);
    }
    for(auto i:p->fdlist){
        i->accept(this);
    }
    if (envfun.count("main"))
    {
        envfun["main"]->cuerpo->accept(this);
    }
    else{
        cout << "no existe main"<< endl;
        exit(0);
    }
    typeEnv.remove_level();
    env.remove_level();
    return 0;
}

string EVALVisitor::getType(Exp* e) {
    if (auto* be = dynamic_cast<BoolExp*>(e)) return "bool";
    if (auto* ne = dynamic_cast<NumberExp*>(e)) return ne->is_float ? "float" : "int";
    if (auto* ie = dynamic_cast<IdExp*>(e)) return typeEnv.lookup(ie->value);
    if (auto* bexp = dynamic_cast<BinaryExp*>(e)) {
        if (bexp->op == LE_OP || bexp->op == AND_OP) return "bool";
        string lt = getType(bexp->left);
        string rt = getType(bexp->right);
        return (lt == "float" || rt == "float") ? "float" : "int";
    }
    if (auto* fe = dynamic_cast<FcallExp*>(e))
        return funReturnTypes[fe->nombre];
    return "int";
}

void EVALVisitor::interprete(Program* programa) {
    if (programa) {
        cout << "Interprete:" << endl;
        programa->accept(this);
    }
}
