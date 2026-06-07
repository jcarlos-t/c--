#include "TypeChecker.h"
#include <iostream>
#include <stdexcept>
using namespace std;


Type* NumberExp::accept(TypeVisitor* v) { return v->visit(this); }
Type* BoolExp::accept(TypeVisitor* v) { return v->visit(this); }
Type* IdExp::accept(TypeVisitor* v) { return v->visit(this); }
Type* BinaryExp::accept(TypeVisitor* v) { return v->visit(this); }
Type* FcallExp::accept(TypeVisitor* v) { return v->visit(this); }

void AssignStm::accept(TypeVisitor* v) { v->visit(this); }
void PrintStm::accept(TypeVisitor* v) { v->visit(this); }
void ReturnStm::accept(TypeVisitor* v) { v->visit(this); }
void WhileStm::accept(TypeVisitor* v) { v->visit(this); }
void IfStm::accept(TypeVisitor* v) { v->visit(this); }

void VarDec::accept(TypeVisitor* v) { v->visit(this); }
void FunDec::accept(TypeVisitor* v) { v->visit(this); }
void Body::accept(TypeVisitor* v) { v->visit(this); }
void Program::accept(TypeVisitor* v) { v->visit(this); }

// ===========================================================
//   Constructor del TypeChecker
// ===========================================================

TypeChecker::TypeChecker() {
    intType = new Type(Type::INT);
    boolType = new Type(Type::BOOL);
    voidType = new Type(Type::VOID);
    floatType = new Type(Type::FLOAT);
}

// ===========================================================
//   Registrar funciones globales
// ===========================================================

void TypeChecker::add_function(FunDec* fd) {
    if (functions.find(fd->nombre) != functions.end()) {
        cerr << "Error: función '" << fd->nombre << "' ya fue declarada." << endl;
        exit(0);
    }

    Type* returnType = new Type();
    if (!returnType->set_basic_type(fd->tipo)) {
        cerr << "Error: tipo de retorno no válido en función '" << fd->nombre << "'." << endl;
        exit(0);
    }

    FuncInfo info;
    info.returnType = returnType;
    for (size_t i = 0; i < fd->Tparametros.size(); ++i) {
        Type* pt = new Type();
        if (!pt->set_basic_type(fd->Tparametros[i])) {
            cerr << "Error: tipo de parámetro inválido en función '" << fd->nombre << "'." << endl;
            exit(0);
        }
        info.paramTypes.push_back(pt);
    }

    functions[fd->nombre] = info;
}

// ===========================================================
//   Método principal de verificación
// ===========================================================

void TypeChecker::typecheck(Program* program) {
    if (program) program->accept(this);
    cout << "Revisión exitosa" << endl;
}

// ===========================================================
//   Nivel superior: Programa y Bloque
// ===========================================================

void TypeChecker::visit(Program* p) {
    // Primero registrar funciones
    for (auto f : p->fdlist)
        add_function(f);

    env.add_level();
    for (auto v : p->vdlist)
        v->accept(this);  
    for (auto f : p->fdlist)
        f->accept(this);  
    env.remove_level();
}

void TypeChecker::visit(Body* b) {
    env.add_level();
    for (auto v : b->vdlist)
        v->accept(this); 
    for (auto s : b->stmlist)
        s->accept(this); 
    env.remove_level();
}

// ===========================================================
//   Declaraciones
// ===========================================================

void TypeChecker::visit(VarDec* v) {
    Type* t = new Type();
    if (!t->set_basic_type(v->tipo)) {
        cerr << "Error: tipo de variable no válido." << endl;
        exit(0);
    }

    for (const auto& id : v->variables) {
        if (env.check(id)) {
            cerr << "Error: variable '" << id << "' ya declarada." << endl;
            exit(0);
        }
        env.add_var(id, t);
    }
}

void TypeChecker::visit(FunDec* f) {
    env.add_level();
    for (size_t i = 0; i < f->Nparametros.size(); ++i) {
        Type* pt = new Type();
        if (!pt->set_basic_type(f->Tparametros[i])) {
            cerr << "Error: tipo de parámetro inválido en función '" << f->nombre << "'." << endl;
            exit(0);
        }
        env.add_var(f->Nparametros[i], pt);
    }
    Type* returnType = new Type();
    returnType->set_basic_type(f->tipo);
    retornodefuncion = returnType;
    f->cuerpo->accept(this);

    env.remove_level();
}

// ===========================================================
//   Sentencias
// ===========================================================

void TypeChecker::visit(PrintStm* stm) {
    Type* t = stm->e->accept(this);
    if (!(t->match(intType) || t->match(boolType) || t->match(floatType))) {
        cerr << "Error: tipo inválido en print (solo int, bool o float)." << endl;
        exit(0);
    }
}

void TypeChecker::visit(AssignStm* stm) {
    if (!env.check(stm->id)) {
        cerr << "Error: variable '" << stm->id << "' no declarada." << endl;
        exit(0);
    }

    Type* varType = env.lookup(stm->id);
    Type* expType = stm->e->accept(this);

    if (!varType->match(expType)) {
        cerr << "Error: tipos incompatibles en asignación a '" << stm->id << "'." << endl;
        exit(0);
    }
}

void TypeChecker::visit(ReturnStm* stm) {
    if (retornodefuncion->match(voidType) && stm->e) {
        cerr << "Error: una función void no debe tener return." << endl;
        exit(0);
    }
    if (stm->e) {
        Type* t = stm->e->accept(this);
        if (!(t->match(intType) || t->match(boolType) || t->match(floatType))) {
            cerr << "Error: tipo inválido en return." << endl;
            exit(0);
        }
        if (!(t->match(retornodefuncion))) {
            cerr << "Error: retorno distinto al tipo declarado de la función." << endl;
            exit(0);
        }
    }
}

// ===========================================================
//   While / If
// ===========================================================

void TypeChecker::visit(WhileStm* stm) {
    Type* t = stm->condition->accept(this);
    if (!t->match(boolType)) {
        cerr << "Error: condición de while debe ser bool." << endl;
        exit(0);
    }
    stm->body->accept(this);
}

void TypeChecker::visit(IfStm* stm) {
    Type* t = stm->condition->accept(this);
    if (!t->match(boolType)) {
        cerr << "Error: condición de if debe ser bool." << endl;
        exit(0);
    }
    stm->thenBody->accept(this);
    if (stm->elseBody)
        stm->elseBody->accept(this);
}

// ===========================================================
//   Expresiones
// ===========================================================

Type* TypeChecker::visit(BinaryExp* e) {
    Type* left = e->left->accept(this);
    Type* right = e->right->accept(this);

    switch (e->op) {
        case PLUS_OP: 
        case MINUS_OP: 
        case MUL_OP: 
        case DIV_OP: 
        case POW_OP:
            if (left->match(intType) && right->match(intType))
                return intType;
            if (left->match(floatType) && right->match(floatType))
                return floatType;
            if ((left->match(intType) && right->match(floatType)) ||
                (left->match(floatType) && right->match(intType)))
                return floatType;
            cerr << "Error: operación aritmética requiere operandos int o float." << endl;
            exit(0);
        case LE_OP:
            if (left->match(intType) && right->match(intType))
                return boolType;
            if (left->match(floatType) && right->match(floatType))
                return boolType;
            cerr << "Error: comparación requiere operandos int o float." << endl;
            exit(0);

        case AND_OP:
            if (!(left->match(boolType) && right->match(boolType))) {
                cerr << "Error: operación lógica requiere operandos bool." << endl;
                exit(0);
            }
            return boolType;

        default:
            cerr << "Error: operador binario no soportado." << endl;
            exit(0);
    }
}

Type* TypeChecker::visit(NumberExp* e) {
    return e->is_float ? floatType : intType;
}

Type* TypeChecker::visit(BoolExp* e) { return boolType; }

Type* TypeChecker::visit(IdExp* e) {
    if (!env.check(e->value)) {
        cerr << "Error: variable '" << e->value << "' no declarada." << endl;
        exit(0);
    }
    return env.lookup(e->value);
}

Type* TypeChecker::visit(FcallExp* e) {
    auto it = functions.find(e->nombre);
    if (it == functions.end()) {
        cerr << "Error: llamada a función no declarada '" << e->nombre << "'." << endl;
        exit(0);
    }

    FuncInfo& info = it->second;

    if (e->argumentos.size() != info.paramTypes.size()) {
        cerr << "Error: cantidad de argumentos inválida en llamada a '" << e->nombre
             << "'." << endl;
        exit(0);
    }

    int i = 0;
    for (auto arg : e->argumentos) {
        Type* argType = arg->accept(this);
        if (!argType->match(info.paramTypes[i])) {
            cerr << "Error: tipo de argumento inválido en posición " << (i+1) << " para llamada a '"
                 << e->nombre << "'." << endl;
            exit(0);
        }
        i++;
    }

    return info.returnType;
}
