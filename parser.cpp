#include <iostream>
#include <stdexcept>
#include "token.h"
#include "scanner.h"
#include "ast.h"
#include "parser.h"

using namespace std;

// =============================
// Métodos de la clase Parser
// =============================

Parser::Parser(Scanner* sc) : scanner(sc) {
    previous = nullptr;
    current = scanner->nextToken();
    if (current->type == Token::ERR) {
        throw runtime_error("Error léxico");
    }
}

bool Parser::match(Token::Type ttype) {
    if (check(ttype)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(Token::Type ttype) {
    if (isAtEnd()) return false;
    return current->type == ttype;
}

bool Parser::advance() {
    if (!isAtEnd()) {
        Token* temp = current;
        if (previous) delete previous;
        current = scanner->nextToken();
        previous = temp;

        if (check(Token::ERR)) {
            throw runtime_error("Error lexico");
        }
        return true;
    }
    return false;
}

bool Parser::isAtEnd() {
    return (current->type == Token::END);
}


// =============================
// Reglas gramaticales
// =============================

Program* Parser::parseProgram() {
    Program* p = new Program();
    
    if(check(Token::VAR)){
        p->vdlist.push_back(parseVarDec());
        match(Token::SEMICOL);
        while(check(Token::VAR)){
            p->vdlist.push_back(parseVarDec());
            match(Token::SEMICOL);
        }
    }

    if(check(Token::FUN)){
        p->fdlist.push_back(parseFunDec());
        while(check(Token::FUN)){
            p->fdlist.push_back(parseFunDec());
        }
    }

    if (!isAtEnd()) {
        throw runtime_error("Error sintáctico");
    }
    
    cout << "Parseo exitoso" << endl;
    return p;
}

VarDec* Parser::parseVarDec(){
    VarDec* vd = new VarDec();
    match(Token::VAR);
    match(Token::ID);
    vd->tipo = previous->text;
    match(Token::ID);
    vd->variables.push_back(previous->text);
    while(match(Token::COMA)){
        match(Token::ID);
        vd->variables.push_back(previous->text);
    }
    return vd;
}

FunDec* Parser::parseFunDec(){
    FunDec* fd = new FunDec();
    match(Token::FUN);
    match(Token::ID);
    fd -> tipo = previous->text;
    match(Token::ID);
    fd -> nombre = previous->text;
    match(Token::LPAREN);
    if(match(Token::ID)){
        fd -> Tparametros.push_back(previous->text);
        match(Token::ID);
        fd -> Nparametros.push_back(previous->text);
        while(match(Token::COMA)){
            match(Token::ID);
            fd -> Tparametros.push_back(previous->text);
            match(Token::ID);
            fd -> Nparametros.push_back(previous->text);
        }
    }
    match(Token::RPAREN);
    match(Token::COLON); // opcional ':'
    fd -> cuerpo = parseBody();
    match(Token::ENDFUN);
    return fd;
}

Body* Parser::parseBody(){
    Body* b = new Body();
    if(check(Token::VAR)){
        b->vdlist.push_back(parseVarDec());
        match(Token::SEMICOL);
        while(check(Token::VAR)){
            b->vdlist.push_back(parseVarDec());
            match(Token::SEMICOL);
        }
    }
    b->stmlist.push_back(parseStm());
    while(match(Token::SEMICOL)){
        b->stmlist.push_back(parseStm());
    }
    return b;
}

Stm* Parser::parseStm() {
    Stm* a;
    Exp* e;
    string variable;
    if(match(Token::ID)){
        variable = previous->text;
        if (check(Token::LPAREN)) {
            FcallExp* fcall = new FcallExp();
            fcall->nombre = variable;
            match(Token::LPAREN);
            if (!check(Token::RPAREN)) {
                fcall->argumentos.push_back(parseCE());
                while(match(Token::COMA)){
                    fcall->argumentos.push_back(parseCE());
                }
            }
            match(Token::RPAREN);
            // FcallExp as a statement: evaluate and discard
            FcallExp* fc = fcall;
            class ExprStm : public Stm {
            public:
                FcallExp* fcall;
                ExprStm(FcallExp* f) : fcall(f) {}
                int accept(Visitor* v) { fcall->accept(v); return 0; }
                void accept(TypeVisitor* v) { fcall->accept(v); }
            };
            return new ExprStm(fcall);
        }
        match(Token::ASSIGN);
        e = parseCE();
        return new AssignStm(variable,e);
    }
    else if(match(Token::PRINT)){
        match(Token::LPAREN);
        e = parseCE();
        match(Token::RPAREN);
        return new PrintStm(e);
    }
    else if(match(Token::RETURN)){
        ReturnStm* r = new ReturnStm();
        if (match(Token::LPAREN)) {
            r->e = parseCE();
            match(Token::RPAREN);
        }
        return r;
    }
    else if(match(Token::WHILE)){
        return parseWhileStm();
    }
    else if(match(Token::IF)){
        return parseIfStm();
    }
    else{
        throw runtime_error("Error sintáctico");
    }
    return a;
}


Stm* Parser::parseWhileStm() {
    match(Token::LPAREN);
    Exp* cond = parseCE();
    match(Token::RPAREN);
    Body* b = parseBody();
    match(Token::ENDWHILE);
    return new WhileStm(cond, b);
}

Stm* Parser::parseIfStm() {
    match(Token::LPAREN);
    Exp* cond = parseCE();
    match(Token::RPAREN);
    Body* thenB = parseBody();
    Body* elseB = nullptr;
    if (match(Token::ELSE)) {
        elseB = parseBody();
    }
    match(Token::ENDIF);
    return new IfStm(cond, thenB, elseB);
}

Exp* Parser::parseCE() {
    Exp* l = parseBE();
    if (match(Token::AND)){
        BinaryOp op;
        op = AND_OP;
        Exp* r = parseBE();
        l = new BinaryExp(l, r, op);
    }
    return l;
}

Exp* Parser::parseBE() {
    Exp* l = parseAE();
    if (match(Token::LE)){
        BinaryOp op;
        op = LE_OP;
        Exp* r = parseAE();
        l = new BinaryExp(l, r, op);
    }
    return l;
}


Exp* Parser::parseAE() {
    Exp* l = parseE();
    while (match(Token::PLUS) || match(Token::MINUS)) {
        BinaryOp op;
        if (previous->type == Token::PLUS){
            op = PLUS_OP;
        }
        else{
            op = MINUS_OP;
        }
        Exp* r = parseE();
        l = new BinaryExp(l, r, op);
    }
    return l;
}



Exp* Parser::parseE() {
    Exp* l = parseT();
    while (match(Token::MUL) || match(Token::DIV)) {
        BinaryOp op;
        if (previous->type == Token::MUL){
            op = MUL_OP;
        }
        else{
            op = DIV_OP;
        }
        Exp* r = parseT();
        l = new BinaryExp(l, r, op);
    }
    return l;
}


Exp* Parser::parseT() {
    Exp* l = parseF();
    if (match(Token::POW)) {
        BinaryOp op = POW_OP;
        Exp* r = parseF();
        l = new BinaryExp(l, r, op);
    }
    return l;
}

Exp* Parser::parseF() {
    Exp* e; 
    if (match(Token::NUM)) {
        bool is_float = (previous->text.find('.') != string::npos);
        return new NumberExp(stod(previous->text), is_float);
    } 
    else if (match(Token::LPAREN))
    {
        e = parseCE();
        match(Token::RPAREN);
        return e;
    }

    else if (match(Token::TRUE))
    {
        BoolExp* b = new BoolExp();
        b->value = 1;
        return b;
    }
    
    else if (match(Token::FALSE))
    {
        BoolExp* b = new BoolExp();
        b->value = 0;
        return b;
    }

    else if (match(Token::ID))
    {   
        string nom = previous->text;
        if (match(Token::LPAREN))
        {
            FcallExp* fcall = new FcallExp();
            fcall->nombre = nom,
            fcall->argumentos.push_back(parseCE());
            while(match(Token::COMA)){
                fcall->argumentos.push_back(parseCE());
            }
            match(Token::RPAREN);
            return fcall;
        }
        else { 
            return new IdExp(nom);
        }
    }

    else {
        throw runtime_error("Error sintáctico");
    }
}
