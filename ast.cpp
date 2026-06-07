#include "ast.h"
#include <iostream>

using namespace std;

// ------------------ Exp ------------------
Exp::~Exp() {}

string Exp::binopToChar(BinaryOp op) {
    switch (op) {
        case PLUS_OP:  return "+";
        case MINUS_OP: return "-";
        case MUL_OP:   return "*";
        case DIV_OP:   return "/";
        case POW_OP:   return "**";
        case LE_OP:   return "<";
        case AND_OP:   return "and";
        default:       return "?";
    }
}

// ------------------ BinaryExp ------------------
BinaryExp::BinaryExp(Exp* l, Exp* r, BinaryOp o)
    : left(l), right(r), op(o) {}

    
BinaryExp::~BinaryExp() {
    delete left;
    delete right;
}



// ------------------ NumberExp ------------------
NumberExp::NumberExp(double v, bool f) : value(v), is_float(f) {}

NumberExp::~NumberExp() {}


// ------------------idExp ------------------
IdExp::IdExp(string v) : value(v) {}

IdExp::~IdExp() {}

Stm::~Stm(){}

PrintStm::~PrintStm(){}

AssignStm::~AssignStm(){}

Program::~Program(){}

PrintStm::PrintStm(Exp* expresion){
    e=expresion;
}

AssignStm::AssignStm(string variable,Exp* expresion){
    id = variable;
    e = expresion;
}

Program::Program(){}

// ------------------ WhileStm ------------------
WhileStm::WhileStm(Exp* c, Body* b) : condition(c), body(b) {}

WhileStm::~WhileStm() {
    delete condition;
    delete body;
}

// ------------------ IfStm ------------------
IfStm::IfStm(Exp* c, Body* t, Body* e) : condition(c), thenBody(t), elseBody(e) {}

IfStm::~IfStm() {
    delete condition;
    delete thenBody;
    delete elseBody;
}
