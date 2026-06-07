#ifndef VISITOR_H
#define VISITOR_H
#include "ast.h"
#include <list>
#include <unordered_map>
#include "environment.h"


class BinaryExp;
class NumberExp;
class Program;
class PrintStm;
class AssignStm;
class FunDec;
class ReturnStm;
class WhileStm;
class IfStm;
class Body;
class VarDec;
class FcallExp;
class BoolExp;

class Visitor {
public:
    virtual double visit(BinaryExp* exp) = 0;
    virtual double visit(NumberExp* exp) = 0;
    virtual double visit(BoolExp* exp) = 0;
    virtual double visit(IdExp* exp) = 0;
    virtual double visit(FcallExp* stm) = 0;
    virtual int visit(Program* p) = 0;
    virtual int visit(PrintStm* stm) = 0;
    virtual int visit(AssignStm* stm) = 0;
    virtual int visit(ReturnStm* stm) = 0;
    virtual int visit(WhileStm* stm) = 0;
    virtual int visit(IfStm* stm) = 0;
    virtual int visit(VarDec* vd) = 0;
    virtual int visit(Body* b) = 0;
    virtual int visit(FunDec* fd) = 0;

};



class PrintVisitor : public Visitor {
public:

    double visit(BinaryExp* exp) override;
    double visit(NumberExp* exp) override;
    double visit(BoolExp* exp) override;
    double visit(IdExp* exp) override;
    double visit(FcallExp* stm) override;
    int visit(Program* p) override ;
    int visit(PrintStm* stm) override;
    int visit(AssignStm* stm) override;
    int visit(ReturnStm* stm) override;
    int visit(WhileStm* stm) override;
    int visit(IfStm* stm) override;
    int visit(VarDec* vd) override;
    int visit(Body* b) override;
    int visit(FunDec* fd) override;

    void imprimir(Program* program); 
};

class EVALVisitor : public Visitor {
public:
    Environment<double> env;
    Environment<string> typeEnv;
    unordered_map<string, FunDec*> envfun;
    unordered_map<string, string> funReturnTypes;
    double retval;
    bool retcall;
    double visit(BinaryExp* exp) override;
    double visit(NumberExp* exp) override;
    double visit(BoolExp* exp) override;
    double visit(IdExp* exp) override;
    double visit(FcallExp* stm) override;
    int visit(Program* p) override ;
    int visit(PrintStm* stm) override;
    int visit(AssignStm* stm) override;
    int visit(ReturnStm* stm) override;
    int visit(WhileStm* stm) override;
    int visit(IfStm* stm) override;
    int visit(VarDec* vd) override;
    int visit(Body* b) override;
    int visit(FunDec* fd) override;
    void interprete(Program* program);
    string getType(Exp* e);
};



#endif // VISITOR_H