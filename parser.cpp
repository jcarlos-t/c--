#include <iostream>
#include <sstream>
#include <stdexcept>
#include "token.h"
#include "scanner.h"
#include "ast.h"
#include "parser.h"

using namespace std;

// =============================
// Constructor y helpers
// =============================

Parser::Parser(Scanner* sc) : scanner(sc) {
    previous = nullptr;
    current = scanner->nextToken();
    if (current->type == Token::ERR) {
        throw runtime_error("Error léxico");
    }
}

// match: si el token actual es ttype, avanza y retorna true
bool Parser::match(Token::Type ttype) {
    if (check(ttype)) {
        advance();
        return true;
    }
    return false;
}

// check: true si el token actual es ttype (sin consumir)
bool Parser::check(Token::Type ttype) const {
    if (isAtEnd()) return false;
    return current->type == ttype;
}

// advance: avanza al siguiente token, libera el anterior
bool Parser::advance() {
    if (!isAtEnd()) {
        Token* temp = current;
        if (previous) delete previous;
        current = scanner->nextToken();
        previous = temp;
        if (check(Token::ERR)) {
            throw runtime_error("Error léxico");
        }
        return true;
    }
    return false;
}

// isAtEnd: true si el token actual es END
bool Parser::isAtEnd() const {
    return current->type == Token::END;
}

// consume: si coincide, avanza; si no, lanza error con msg
Token* Parser::consume(Token::Type ttype, const string& msg) {
    if (check(ttype)) {
        Token* t = current;
        advance();
        return t;
    }
    ostringstream oss;
    oss << "line " << current->line << ":" << current->col << " - " << msg
        << " (se esperaba " << ttype << " pero se encontró '" << current->text << "')";
    throw runtime_error(oss.str());
}

// sync_error: lanza error sintáctico con línea/columna
void Parser::sync_error(const string& msg) {
    ostringstream oss;
    oss << "line " << current->line << ":" << current->col << " - Error sintáctico: " << msg;
    throw runtime_error(oss.str());
}

// Helper: is_type_start

// is_type_keyword: true si el token es un tipo primitivo (sin struct)
static bool is_type_keyword(Token::Type t) {
    return t == Token::VOID || t == Token::INT || t == Token::CHAR ||
           t == Token::FLOAT || t == Token::DOUBLE || t == Token::BOOL ||
           t == Token::AUTO;
}

// kind_from_token: convierte Token::Type a PrimitiveTypeNode::Prim
static PrimitiveTypeNode::Prim kind_from_token(Token::Type t) {
    switch (t) {
        case Token::VOID:   return PrimitiveTypeNode::VOID;
        case Token::INT:    return PrimitiveTypeNode::INT;
        case Token::CHAR:   return PrimitiveTypeNode::CHAR;
        case Token::FLOAT:  return PrimitiveTypeNode::FLOAT;
        case Token::DOUBLE: return PrimitiveTypeNode::DOUBLE;
        case Token::BOOL:   return PrimitiveTypeNode::BOOL;
        default:            return PrimitiveTypeNode::AUTO;
    }
}

// is_type_start: true si el token actual puede iniciar un tipo
bool Parser::is_type_start() const {
    if (isAtEnd()) return false;
    return is_type_keyword(current->type) || check(Token::STRUCT);
}

// can_start_type: true si el siguiente token puede ser tipo o tipo temlate ID<>
bool Parser::can_start_type() {
    if (is_type_start()) return true;
    if (check(Token::ID)) {
        Scanner::Pos saved = scanner->getPos();
        Token* next = scanner->nextToken();
        bool result = (next->type == Token::LT);
        delete next;
        scanner->setPos(saved);
        return result;
    }
    return false;
}

// rollback: restaura scanner y reconstruye estado current/previous
void Parser::rollback(Scanner::Pos saved) {
    scanner->setPos(saved);
    delete current;
    delete previous;
    previous = nullptr;
    current = scanner->nextToken();
}

// parse_array_suffix: parsea [expr] opcional en variables/parámetros
void Parser::parse_array_suffix(VarDecl* vd) {
    while (match(Token::LBRACKET)) {
        if (!check(Token::RBRACKET))
            vd->array_sizes.push_back(parse_expression());
        consume(Token::RBRACKET, "Se esperaba ']'");
    }
}

// =============================
// Types
// =============================

// parse_type: parsea tipo básico + cero o más '*' (punteros)
TypeNode* Parser::parse_type() {
    TypeNode* base = parse_basic_type();
    while (match(Token::STAR)) {
        base = new PointerTypeNode(base);
    }
    return base;
}

// parse_basic_type: parsea void/int/char/float/double/bool/auto/struct/template
TypeNode* Parser::parse_basic_type() {
    if (is_type_keyword(current->type)) {
        Token::Type t = current->type;
        advance();
        return new PrimitiveTypeNode(kind_from_token(t));
    }
    // struct person <- esto es un tipo
    if (match(Token::STRUCT)) {
        Token* name = consume(Token::ID, "Se esperaba nombre de struct");
        if (match(Token::LT)) {
            vector<TypeNode*> targs;
            do {
                targs.push_back(parse_type());
            } while (match(Token::COMA));
            consume(Token::GT, "Se esperaba '>' en instanciación de template");
            return new TemplateTypeNode(name->text, targs);
        }
        return new StructTypeNode(name->text);
    }
    // para tipos por template
    if (check(Token::ID)) {
        string tname = current->text;
        // hay tipo template con ese id
        if (current_template_params.count(tname)) {
            advance();
            return new NamedTypeNode(tname);
        }
        Scanner::Pos saved = scanner->getPos();
        advance();
        // instancia concreta de template: Nombre<T1, T2, ...>
        if (match(Token::LT)) {
            vector<TypeNode*> targs;
            do {
                targs.push_back(parse_type());
            } while (match(Token::COMA));
            consume(Token::GT, "Se esperaba '>' en instanciación de template");
            return new TemplateTypeNode(tname, targs);
        }
        // rollback: no era un template, puede ser un id
        rollback(saved);
    }
    sync_error("Se esperaba un tipo (int, float, void, char, double, auto, struct)");
    return nullptr;
}

// =============================
// Program y Declarations
// =============================

Program* Parser::parse_program() {
    Program* p = new Program();
    while (!isAtEnd()) {
        parse_declaration(p);
    }
    return p;
}

Program* Parser::parseProgram() {
    Program* p = parse_program();
    return p;
}

// parse_declaration: parsea template, struct, función o variable global
void Parser::parse_declaration(Program* p) {
    // Template declaration: "template" "<" template_parameter_list ">"
    //                      ( function_declaration | struct_declaration )
    if (match(Token::TEMPLATE)) {
        consume(Token::LT, "Se esperaba '<' después de template");
        vector<string> tparams;
        do {
            consume(Token::TYPENAME, "Se esperaba 'typename'");
            Token* tname = consume(Token::ID, "Se esperaba nombre del parámetro de template");
            tparams.push_back(tname->text);
            current_template_params.insert(tname->text);
        } while (match(Token::COMA));
        consume(Token::GT, "Se esperaba '>' después de template");

        // template<> strunct
        if (check(Token::STRUCT)) {
            StructDecl* sd = parse_struct_decl();
            TemplateDecl* td = new TemplateDecl(tparams, sd);
            p->templates.push_back(td);
        // template<> function
        } else {
            TypeNode* ttype = parse_type();
            Token* tname = consume(Token::ID, "Se esperaba nombre de función");
            FunDecl* fd = parse_function_decl(ttype, tname->text);
            fd->is_template = true;
            TemplateDecl* td = new TemplateDecl(tparams, fd);
            p->templates.push_back(td);
        }

        for (auto tp : tparams)
            current_template_params.erase(tp);
        return;
    }

    // Struct declaration: "struct" ID "{" { variable_declaration } "}" ";"
    if (check(Token::STRUCT)) {
        Scanner::Pos saved = scanner->getPos();
        advance(); // consume STRUCT
        if (check(Token::ID)) {
            string sname = current->text;
            advance(); // consume ID
            if (check(Token::LBRACE)) {
                // Struct declaration
                StructDecl* sd = new StructDecl(sname);
                consume(Token::LBRACE, "Se esperaba '{'");
                while (!check(Token::RBRACE) && !isAtEnd()) {
                    TypeNode* mt = parse_type();
                    Token* mn = consume(Token::ID, "Se esperaba nombre del miembro");
                    VarDecl* member = parse_variable_decl(mt, mn->text);
                    sd->members.push_back(member);
                }
                consume(Token::RBRACE, "Se esperaba '}'");
                consume(Token::SEMICOL, "Se esperaba ';' después de struct");
                p->structs.push_back(sd);
                return;
            }
        }
        // rollback: no era declaracion de struct, puede ser instanciación: struct name x;
        rollback(saved);
    }

    // funct | vardec
    TypeNode* type = parse_type();

    if (check(Token::ID)) {
        Token* name = current;
        string id_name = name->text;
        advance();

        if (check(Token::LPAREN)) {
            // Function declaration: type IDENTIFIER "(" parameter_list ")" compound_statement
            FunDecl* fd = parse_function_decl(type, id_name);
            p->functions.push_back(fd);
        } else {
            // Variable declaration: type IDENTIFIER array_suffix [ "=" expression ] ";"
            VarDecl* vd = parse_variable_decl(type, id_name);
            p->globals.push_back(vd);
        }
    } else {
        sync_error("Se esperaba un identificador después del tipo");
    }
}

// =============================
// Function declaration
// =============================

// parse_function_decl: parsea params y cuerpo de función
FunDecl* Parser::parse_function_decl(Exp* ret_type, const string& name) {
    FunDecl* fd = new FunDecl(ret_type, name, nullptr);
    fd->loc.line = current->line; fd->loc.column = current->col;
    consume(Token::LPAREN, "Se esperaba '(' en declaración de función");

    if (!check(Token::RPAREN)) {
        fd->params.push_back(parse_parameter());
        while (match(Token::COMA)) {
            fd->params.push_back(parse_parameter());
        }
    }
    consume(Token::RPAREN, "Se esperaba ')' en declaración de función");

    Body* body = dynamic_cast<Body*>(parse_compound_statement());
    if (!body) sync_error("Se esperaba cuerpo de función compuesto por { }");
    fd->body = body;
    return fd;
}

// parse_variable_decl: parsea arreglos opcionales e inicializador
VarDecl* Parser::parse_variable_decl(Exp* type, const string& name) {
    VarDecl* vd = new VarDecl(type, name);
    vd->loc.line = current->line; vd->loc.column = current->col;

    // array_suffix
    parse_array_suffix(vd);

    // optional initializer
    if (match(Token::ASSIGN)) {
        if (match(Token::LBRACE)) {
            if (!match(Token::RBRACE)) {
                vd->init_list.push_back(parse_expression());
                while (match(Token::COMA)) {
                    vd->init_list.push_back(parse_expression());
                }
                consume(Token::RBRACE, "Se esperaba '}' despues de la lista de inicialización");
            }
        } else {
            vd->initializer = parse_expression();
        }
    }

    consume(Token::SEMICOL, "Se esperaba ';' al final de la declaración");
    return vd;
}

// =============================
// Struct declaration
// =============================

// parse_struct_decl: parsea struct ID { miembros }
StructDecl* Parser::parse_struct_decl() {
    consume(Token::STRUCT, "Se esperaba 'struct'");
    Token* name = consume(Token::ID, "Se esperaba nombre del struct");
    StructDecl* sd = new StructDecl(name->text);
    consume(Token::LBRACE, "Se esperaba '{' en declaración de struct");

    while (!check(Token::RBRACE) && !isAtEnd()) {
        TypeNode* member_type = parse_type();
        Token* member_name = consume(Token::ID, "Se esperaba nombre del miembro");
        VarDecl* member = parse_variable_decl(member_type, member_name->text);
        sd->members.push_back(member);
    }
    consume(Token::RBRACE, "Se esperaba '}' en declaración de struct");
    consume(Token::SEMICOL, "Se esperaba ';' después de declaración de struct");
    return sd;
}

// =============================
// Parameters
// =============================

// parse_parameter: parsea type id [array_suffix] para parámetros
VarDecl* Parser::parse_parameter() {
    TypeNode* type = parse_type();
    Token* name = consume(Token::ID, "Se esperaba nombre del parámetro");
    VarDecl* param = new VarDecl(type, name->text);
    parse_array_suffix(param);
    return param;
}


// =============================
// Statements
// =============================

// parse_statement: parsea cualquier sentencia (if, while, for, switch, break, continue, return, free, expr, decl local)
Stm* Parser::parse_statement() {
    if (can_start_type()) {
        return parse_local_var_decl();
    }
    // body {....}
    if (check(Token::LBRACE))
        return parse_compound_statement();
    if (check(Token::IF))
        return parse_if_statement();
    if (check(Token::WHILE))
        return parse_while_statement();
    if (check(Token::DO))
        return parse_do_while_statement();
    if (check(Token::FOR))
        return parse_for_statement();
    if (check(Token::SWITCH))
        return parse_switch_statement();
    if (check(Token::BREAK)) {
        int bl = current->line, bc = current->col;
        advance();
        consume(Token::SEMICOL, "Se esperaba ';' después de break");
        BreakStmt* bs = new BreakStmt();
        bs->loc.line = bl; bs->loc.column = bc;
        return bs;
    }
    if (check(Token::CONTINUE)) {
        int cl = current->line, cc = current->col;
        advance();
        consume(Token::SEMICOL, "Se esperaba ';' después de continue");
        ContinueStmt* cs = new ContinueStmt();
        cs->loc.line = cl; cs->loc.column = cc;
        return cs;
    }
    if (check(Token::RETURN))
        return parse_return_statement();

    if (match(Token::FREE)) {
        consume(Token::LPAREN, "Se esperaba '(' después de free");
        Exp* fexpr = parse_expression();
        consume(Token::RPAREN, "Se esperaba ')'");
        consume(Token::SEMICOL, "Se esperaba ';' después de free");
        return new FreeStmt(fexpr);
    }

    // expression_statement: [ expression ] ";"
    if (check(Token::SEMICOL)) {
        advance();
        return new ExprStmtNode(nullptr); // empty statement
    }

    Exp* expr = parse_expression();
    consume(Token::SEMICOL, "Se esperaba ';' al final de la sentencia");
    return new ExprStmtNode(expr);
}

// parse_local_var_decl: parsea declaración de variable local
VarDecl* Parser::parse_local_var_decl() {
    TypeNode* type = parse_type();
    if (auto* st = dynamic_cast<StructTypeNode*>(type)) {
        if (match(Token::LT)) {
            vector<TypeNode*> targs;
            do {
                targs.push_back(parse_type());
            } while (match(Token::COMA));
            consume(Token::GT, "Se esperaba '>' en instanciación de template");
            type = new TemplateTypeNode(st->name, targs);
            delete st;
        }
    }
    Token* name = consume(Token::ID, "Se esperaba identificador");
    return parse_variable_decl(type, name->text);
}

// parse_compound_statement: parsea { stmts }
Stm* Parser::parse_compound_statement() {
    Body* cs = new Body();
    consume(Token::LBRACE, "Se esperaba '{'");
    while (!check(Token::RBRACE) && !isAtEnd()) {
        cs->stmts.push_back(parse_statement());
    }
    consume(Token::RBRACE, "Se esperaba '}'");
    return cs;
}

// parse_if_statement: parsea if (cond) stmt [else stmt]
Stm* Parser::parse_if_statement() {
    consume(Token::IF, "Se esperaba 'if'");
    consume(Token::LPAREN, "Se esperaba '(' después de if");
    Exp* cond = parse_expression();
    consume(Token::RPAREN, "Se esperaba ')' después de condición");
    Stm* then_branch = parse_statement();
    Stm* else_branch = nullptr;
    if (match(Token::ELSE)) {
        else_branch = parse_statement();
    }
    return new IfStmt(cond, then_branch, else_branch);
}

// parse_while_statement: parsea while (cond) stmt
Stm* Parser::parse_while_statement() {
    consume(Token::WHILE, "Se esperaba 'while'");
    consume(Token::LPAREN, "Se esperaba '(' después de while");
    Exp* cond = parse_expression();
    consume(Token::RPAREN, "Se esperaba ')' después de condición");
    Stm* body = parse_statement();
    return new WhileStmt(cond, body);
}

// parse_do_while_statement: parsea do stmt while (cond);
Stm* Parser::parse_do_while_statement() {
    consume(Token::DO, "Se esperaba 'do'");
    Stm* body = parse_statement();
    consume(Token::WHILE, "Se esperaba 'while' después del cuerpo de do");
    consume(Token::LPAREN, "Se esperaba '('");
    Exp* cond = parse_expression();
    consume(Token::RPAREN, "Se esperaba ')'");
    consume(Token::SEMICOL, "Se esperaba ';' después de do-while");
    return new DoWhileStmt(body, cond);
}

// parse_for_statement: parsea for (init; cond; inc) stmt
Stm* Parser::parse_for_statement() {
    consume(Token::FOR, "Se esperaba 'for'");
    consume(Token::LPAREN, "Se esperaba '(' después de for");

    Stm* init = nullptr;
    if (!check(Token::SEMICOL)) {
        if (can_start_type()) {
            init = parse_local_var_decl();
        } else {
            Exp* e = parse_expression();
            init = new ExprStmtNode(e);
        }
    }
    consume(Token::SEMICOL, "Se esperaba ';' en for");

    Exp* condition = nullptr;
    if (!check(Token::SEMICOL)) {
        condition = parse_expression();
    }
    consume(Token::SEMICOL, "Se esperaba ';' en for");

    Exp* increment = nullptr;
    if (!check(Token::RPAREN)) {
        increment = parse_expression();
    }
    consume(Token::RPAREN, "Se esperaba ')' después de for");

    Stm* body = parse_statement();
    return new ForStmt(init, condition, increment, body);
}

// parse_switch_statement: parsea switch (expr) { case/default: ... }
Stm* Parser::parse_switch_statement() {
    consume(Token::SWITCH, "Se esperaba 'switch'");
    consume(Token::LPAREN, "Se esperaba '(' después de switch");
    Exp* expr = parse_expression();
    consume(Token::RPAREN, "Se esperaba ')'");
    consume(Token::LBRACE, "Se esperaba '{'");

    SwitchStmt* ss = new SwitchStmt(expr);
    while (check(Token::CASE) || check(Token::DEFAULT)) {
        if (match(Token::CASE)) {
            Exp* val = parse_expression();
            consume(Token::COLON, "Se esperaba ':' después de case");
            CaseClause* cc = new CaseClause(val);
            // Parse statements until next case/default or }
            while (!check(Token::CASE) && !check(Token::DEFAULT) && !check(Token::RBRACE) && !isAtEnd())
                cc->body.push_back(parse_statement());
            ss->cases.push_back(cc);
        } else if (match(Token::DEFAULT)) {
            consume(Token::COLON, "Se esperaba ':' después de default");
            while (!check(Token::CASE) && !check(Token::DEFAULT) && !check(Token::RBRACE) && !isAtEnd())
                ss->default_body.push_back(parse_statement());
        }
    }
    consume(Token::RBRACE, "Se esperaba '}'");
    return ss;
}

// parse_return_statement: parsea return [expr];
Stm* Parser::parse_return_statement() {
    consume(Token::RETURN, "Se esperaba 'return'");
    int rl = previous->line, rc = previous->col;
    Exp* expr = nullptr;
    if (!check(Token::SEMICOL)) {
        expr = parse_expression();
    }
    consume(Token::SEMICOL, "Se esperaba ';' después de return");
    ReturnStmt* rs = new ReturnStmt(expr);
    rs->loc.line = rl; rs->loc.column = rc;
    return rs;
}

// =============================
// Expressions
// =============================

// parse_expression: punto de entrada para expresiones
Exp* Parser::parse_expression() {
    return parse_assignment();
}

// parse_assignment: parsea = (asociativo a derecha)
Exp* Parser::parse_assignment() {
    Exp* l = parse_logical_or();
    if (match(Token::ASSIGN)) {
        Exp* r = parse_assignment();
        l = new AssignmentNode(l, r, AssignOp::ASSIGN);
    }
    return l;
}

// parse_logical_or: parsea ||
Exp* Parser::parse_logical_or() {
    Exp* l = parse_logical_and();
    while (match(Token::OR)) {
        Exp* r = parse_logical_and();
        l = new BinaryOpNode(l, r, BinaryOp::LOG_OR);
    }
    return l;
}

// parse_logical_and: parsea &&
Exp* Parser::parse_logical_and() {
    Exp* l = parse_equality();
    while (match(Token::AND)) {
        Exp* r = parse_equality();
        l = new BinaryOpNode(l, r, BinaryOp::LOG_AND);
    }
    return l;
}

// parse_equality: parsea == !=
Exp* Parser::parse_equality() {
    Exp* l = parse_relational();
    while (true) {
        if (match(Token::EQ)) {
            Exp* r = parse_relational();
            l = new BinaryOpNode(l, r, BinaryOp::EQ);
        } else if (match(Token::NE)) {
            Exp* r = parse_relational();
            l = new BinaryOpNode(l, r, BinaryOp::NE);
        } else break;
    }
    return l;
}

// parse_relational: parsea < > <= >=
Exp* Parser::parse_relational() {
    Exp* l = parse_additive();
    while (true) {
        if (match(Token::LT)) {
            Exp* r = parse_additive();
            l = new BinaryOpNode(l, r, BinaryOp::LT);
        } else if (match(Token::GT)) {
            Exp* r = parse_additive();
            l = new BinaryOpNode(l, r, BinaryOp::GT);
        } else if (match(Token::LE)) {
            Exp* r = parse_additive();
            l = new BinaryOpNode(l, r, BinaryOp::LE);
        } else if (match(Token::GE)) {
            Exp* r = parse_additive();
            l = new BinaryOpNode(l, r, BinaryOp::GE);
        } else break;
    }
    return l;
}

// parse_additive: parsea + -
Exp* Parser::parse_additive() {
    Exp* l = parse_multiplicative();
    while (true) {
        if (match(Token::PLUS)) {
            Exp* r = parse_multiplicative();
            l = new BinaryOpNode(l, r, BinaryOp::ADD);
        } else if (match(Token::MINUS)) {
            Exp* r = parse_multiplicative();
            l = new BinaryOpNode(l, r, BinaryOp::SUB);
        } else break;
    }
    return l;
}

// parse_multiplicative: parsea * / %
Exp* Parser::parse_multiplicative() {
    Exp* l = parse_pow();
    while (true) {
        if (match(Token::STAR)) {
            Exp* r = parse_pow();
            l = new BinaryOpNode(l, r, BinaryOp::MUL);
        } else if (match(Token::DIV)) {
            Exp* r = parse_pow();
            l = new BinaryOpNode(l, r, BinaryOp::DIV);
        } else if (match(Token::MOD)) {
            Exp* r = parse_pow();
            l = new BinaryOpNode(l, r, BinaryOp::MOD);
        } else break;
    }
    return l;
}

// parse_pow: parsea ^ (potencia, asociativo a derecha)
Exp* Parser::parse_pow() {
    Exp* l = parse_unary();
    if (match(Token::POW)) {
        Exp* r = parse_pow();
        l = new BinaryOpNode(l, r, BinaryOp::POW);
    }
    return l;
}

// parse_unary: parsea ++x --x & * - ! (prefijo)
Exp* Parser::parse_unary() {
    if (match(Token::INC)) {
        Exp* operand = parse_unary();
        return new UnaryOpNode(operand, UnaryOp::PRE_INC);
    }
    if (match(Token::DEC)) {
        Exp* operand = parse_unary();
        return new UnaryOpNode(operand, UnaryOp::PRE_DEC);
    }
    if (match(Token::AMPERSAND)) {
        Exp* operand = parse_unary();
        return new UnaryOpNode(operand, UnaryOp::ADDR);
    }
    if (match(Token::STAR)) {
        Exp* operand = parse_unary();
        return new UnaryOpNode(operand, UnaryOp::DEREF);
    }
    if (match(Token::MINUS)) {
        Exp* operand = parse_unary();
        return new UnaryOpNode(operand, UnaryOp::MINUS);
    }
    if (match(Token::NOT)) {
        Exp* operand = parse_unary();
        return new UnaryOpNode(operand, UnaryOp::LOG_NOT);
    }
    return parse_postfix();
}

// parse_postfix: parsea [] () . -> x++ x-- (postfijo)
Exp* Parser::parse_postfix() {
    Exp* l = parse_primary();

    while (true) {
        if (match(Token::LBRACKET)) {
            Exp* index = parse_expression();
            consume(Token::RBRACKET, "Se esperaba ']'");
            l = new IndexNode(l, index);
        } else if (check(Token::LT) && dynamic_cast<IdentifierNode*>(l)) {
            Scanner::Pos saved = scanner->getPos();
            Token* lookahead = scanner->nextToken();
            Token::Type laType = lookahead->type;
            delete lookahead;
            scanner->setPos(saved);

            bool isTemplate = is_type_keyword(laType) || laType == Token::STRUCT;
            if (!isTemplate && laType == Token::ID) {
                Token* idTok = scanner->nextToken();
                Token* after = scanner->nextToken();
                isTemplate = after && (after->type == Token::GT || after->type == Token::COMA);
                delete idTok;
                delete after;
                scanner->setPos(saved);
            }
            if (!isTemplate) {
                break;
            }

            advance(); // consume '<'
            vector<TypeNode*> targs;
            bool ok = true;
            try {
                targs.push_back(parse_type());
                while (match(Token::COMA)) {
                    targs.push_back(parse_type());
                }
                ok = ok && match(Token::GT);
                ok = ok && check(Token::LPAREN);
            } catch (...) {
                ok = false;
                for (auto t : targs) delete t;
                targs.clear();
            }
            if (ok) {
                match(Token::LPAREN); // consume '('
                FcallNode* call = new FcallNode(l);
                call->template_args = targs;
                if (!check(Token::RPAREN)) {
                    call->args.push_back(parse_assignment());
                    while (match(Token::COMA)) {
                        call->args.push_back(parse_assignment());
                    }
                }
                consume(Token::RPAREN, "Se esperaba ')' en llamada a función");
                l = call;
            } else {
                rollback(saved);
                break;
            }
        } else if (match(Token::LPAREN)) {
            FcallNode* call = new FcallNode(l);
            if (!check(Token::RPAREN)) {
                call->args.push_back(parse_assignment());
                while (match(Token::COMA)) {
                    call->args.push_back(parse_assignment());
                }
            }
            consume(Token::RPAREN, "Se esperaba ')' en llamada a función");
            l = call;
        } else if (match(Token::DOT)) {
            Token* name = consume(Token::ID, "Se esperaba nombre de miembro");
            l = new MemberAccessNode(l, name->text);
        } else if (match(Token::ARROW)) {
            Token* name = consume(Token::ID, "Se esperaba nombre de miembro");
            l = new ArrowAccessNode(l, name->text);
        } else if (match(Token::INC)) {
            l = new UnaryOpNode(l, UnaryOp::POST_INC);
        } else if (match(Token::DEC)) {
            l = new UnaryOpNode(l, UnaryOp::POST_DEC);
        } else break;
    }
    return l;
}

// parse_primary: parsea literales, id, lambdas, (expr), malloc, sizeof, printf
Exp* Parser::parse_primary() {
    if (match(Token::TRUE)) {
        return new BoolLiteralNode(true);
    }
    if (match(Token::FALSE)) {
        return new BoolLiteralNode(false);
    }
    if (match(Token::ID)) {
        return new IdentifierNode(previous->text);
    }
    if (match(Token::NUM)) {
        return new IntegerLiteralNode(stoll(previous->text));
    }
    if (match(Token::FNUM)) {
        return new FloatLiteralNode(stod(previous->text));
    }
    if (match(Token::CHAR_LIT)) {
        string t = previous->text;
        char val = t.size() >= 2 ? t[1] : 0;
        if (t.size() >= 2 && t[1] == '\\') {
            if (t.size() >= 3) {
                switch (t[2]) {
                    case 'n': val = '\n'; break;
                    case 't': val = '\t'; break;
                    case 'r': val = '\r'; break;
                    case '0': val = '\0'; break;
                    case '\\': val = '\\'; break;
                    case '\'': val = '\''; break;
                    case '"': val = '"'; break;
                    default: val = t[2];
                }
            }
        }
        return new CharLiteralNode(val);
    }
    if (match(Token::STRING_LIT)) {
        return new StringLiteralNode(previous->text.substr(1, previous->text.size() - 2));
    }
    // Lambda expression: "[" [ capture_list ] "]" "(" parameter_list ")" "->" type compound_statement
    if (match(Token::LBRACKET)) {
        vector<CaptureNode*> captures;
        if (!check(Token::RBRACKET)) {
            do {
                if (match(Token::AMPERSAND)) {
                    if (check(Token::ID)) {
                        string cname = current->text;
                        advance();
                        captures.push_back(new CaptureNode(CaptureNode::BY_REF, cname));
                    } else {
                        captures.push_back(new CaptureNode(CaptureNode::BY_REF, ""));
                    }
                } else if (match(Token::ASSIGN)) {
                    captures.push_back(new CaptureNode(CaptureNode::BY_VALUE, ""));
                } else if (check(Token::ID)) {
                    string cname = current->text;
                    advance();
                    captures.push_back(new CaptureNode(CaptureNode::BY_VALUE, cname));
                } else {
                    sync_error("Se esperaba captura en lambda");
                }
            } while (match(Token::COMA));
        }
        consume(Token::RBRACKET, "Se esperaba ']' en lambda");
        consume(Token::LPAREN, "Se esperaba '(' en lambda");
        vector<VarDecl*> lparams;
        if (!check(Token::RPAREN)) {
            lparams.push_back(parse_parameter());
            while (match(Token::COMA)) {
                lparams.push_back(parse_parameter());
            }
        }
        consume(Token::RPAREN, "Se esperaba ')' en lambda");
        consume(Token::ARROW, "Se esperaba '->' en lambda");
        TypeNode* lret = parse_type();
        Body* lbody = dynamic_cast<Body*>(parse_compound_statement());
        if (!lbody) sync_error("Se esperaba cuerpo compuesto en lambda");
        return new LambdaExprNode(captures, lparams, lret, lbody);
    }
    if (match(Token::LPAREN)) {
        Exp* expr = parse_expression();
        consume(Token::RPAREN, "Se esperaba ')'");
        return expr;
    }
    if (match(Token::MALLOC)) {
        consume(Token::LPAREN, "Se esperaba '(' después de malloc");
        Exp* size = parse_expression();
        consume(Token::RPAREN, "Se esperaba ')'");
        return new MallocNode(size);
    }
    if (match(Token::SIZEOF)) {
        consume(Token::LPAREN, "Se esperaba '(' después de sizeof");
        Exp* target = parse_type();
        consume(Token::RPAREN, "Se esperaba ')'");
        return new SizeOfNode(target);
    }
    if (match(Token::PRINTF)) {
        consume(Token::LPAREN, "Se esperaba '(' después de print/printf");
        PrintfNode* pn = new PrintfNode();
        if (!check(Token::RPAREN)) {
            // Verificar si el primer argumento es un string literal (formato)
            if (check(Token::STRING_LIT)) {
                StringLiteralNode* fmt = dynamic_cast<StringLiteralNode*>(parse_assignment());
                if (fmt) {
                    pn->format = fmt->value;
                    delete fmt;  // No necesitamos el nodo, solo el string
                }
                // Si hay más argumentos después del formato
                if (match(Token::COMA)) {
                    pn->args.push_back(parse_assignment());
                    while (match(Token::COMA))
                        pn->args.push_back(parse_assignment());
                }
            } else {
                // Sin formato explícito, usar el primer argumento como valor con formato por defecto
                pn->args.push_back(parse_assignment());
                while (match(Token::COMA))
                    pn->args.push_back(parse_assignment());
            }
        }
        consume(Token::RPAREN, "Se esperaba ')'");
        return pn;
    }
    sync_error("Se esperaba una expresión");
    return nullptr;
}
