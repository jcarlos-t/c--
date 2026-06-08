#include <iostream>
#include <sstream>
#include <stdexcept>
#include "token.h"
#include "scanner.h"
#include "ast.h"
#include "parser.h"

using namespace std;

// =============================
// Constructor & helpers
// =============================

Parser::Parser(Scanner* sc) : scanner(sc) {
    previous = nullptr;
    current = scanner->nextToken();
    current_line = 1;
    current_column = 1;
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

bool Parser::check(Token::Type ttype) const {
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
            throw runtime_error("Error léxico");
        }
        return true;
    }
    return false;
}

bool Parser::isAtEnd() const {
    return current->type == Token::END;
}

Token* Parser::consume(Token::Type ttype, const string& msg) {
    if (check(ttype)) {
        Token* t = current;
        advance();
        return t;
    }
    ostringstream oss;
    oss << msg << " (se esperaba " << ttype << " pero se encontró '" << current->text << "')";
    throw runtime_error(oss.str());
}

void Parser::sync_error(const string& msg) {
    ostringstream oss;
    oss << "Error sintáctico: " << msg;
    throw runtime_error(oss.str());
}

// =============================
// Types (grammar.md §3)
// =============================

TypeNode* Parser::parse_type() {
    TypeNode* base = parse_basic_type();
    while (match(Token::STAR)) {
        base = new PointerTypeNode(base);
    }
    return base;
}

TypeNode* Parser::parse_basic_type() {
    if (match(Token::VOID))
        return new PrimitiveTypeNode(PrimitiveTypeNode::VOID);
    if (match(Token::INT))
        return new PrimitiveTypeNode(PrimitiveTypeNode::INT);
    if (match(Token::CHAR))
        return new PrimitiveTypeNode(PrimitiveTypeNode::CHAR);
    if (match(Token::FLOAT))
        return new PrimitiveTypeNode(PrimitiveTypeNode::FLOAT);
    if (match(Token::DOUBLE))
        return new PrimitiveTypeNode(PrimitiveTypeNode::DOUBLE);
    if (match(Token::BOOL))
        return new PrimitiveTypeNode(PrimitiveTypeNode::BOOL);
    if (match(Token::AUTO))
        return new PrimitiveTypeNode(PrimitiveTypeNode::AUTO);
    if (match(Token::STRUCT)) {
        Token* name = consume(Token::ID, "Se esperaba nombre de struct");
        return new StructTypeNode(name->text);
    }
    sync_error("Se esperaba un tipo (int, float, void, char, double, auto, struct)");
    return nullptr;
}

// =============================
// Program & Declarations (grammar.md §1-2)
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
    cout << "Parseo exitoso" << endl;
    return p;
}

void Parser::parse_declaration(Program* p) {
    // Template declaration: "template" "<" template_parameter_list ">"
    //                      ( function_declaration | struct_declaration )
    if (match(Token::TEMPLATE)) {
        consume(Token::LT, "Se esperaba '<' después de template");
        vector<TemplateParam*> tparams;
        do {
            consume(Token::TYPENAME, "Se esperaba 'typename'");
            Token* tname = consume(Token::ID, "Se esperaba nombre del parámetro de template");
            tparams.push_back(new TemplateParam(tname->text));
        } while (match(Token::COMA));
        consume(Token::GT, "Se esperaba '>' después de template");

        // Parse inner declaration (function or struct)
        if (check(Token::STRUCT)) {
            StructDecl* sd = parse_struct_decl();
            TemplateDecl* td = new TemplateDecl(tparams, sd);
            p->templates.push_back(td);
        } else {
            TypeNode* ttype = parse_type();
            Token* tname = consume(Token::ID, "Se esperaba nombre de función");
            FunDecl* fd = parse_function_decl(ttype, tname->text);
            fd->is_template = true;
            TemplateDecl* td = new TemplateDecl(tparams, fd);
            p->templates.push_back(td);
        }
        return;
    }

    // Struct declaration: "struct" ID "{" { variable_declaration } "}" ";"
    // Check by looking ahead without consuming: STRUCT ID LBRACE
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
        // Not a struct declaration — restore scanner and fall through to parse_type
        // Note: parser state (current/previous) is corrupt, so we rebuild by re-scanning
        scanner->setPos(saved);
        delete current;
        delete previous;
        previous = nullptr;
        current = scanner->nextToken();
    }

    // Parse type first
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
// Function declaration (grammar.md §2)
// =============================

FunDecl* Parser::parse_function_decl(Exp* ret_type, const string& name) {
    FunDecl* fd = new FunDecl(ret_type, name, nullptr);
    consume(Token::LPAREN, "Se esperaba '(' en declaración de función");

    if (!check(Token::RPAREN)) {
        fd->params.push_back(parse_parameter());
        while (match(Token::COMA)) {
            fd->params.push_back(parse_parameter());
        }
    }
    consume(Token::RPAREN, "Se esperaba ')' en declaración de función");

    CompoundStmt* body = dynamic_cast<CompoundStmt*>(parse_compound_statement());
    if (!body) sync_error("Se esperaba cuerpo de función compuesto por { }");
    fd->body = body;
    return fd;
}

// =============================
// Variable declaration (grammar.md §2)
// =============================

VarDecl* Parser::parse_variable_decl(Exp* type, const string& name) {
    VarDecl* vd = new VarDecl(type, name);

    // array_suffix
    while (match(Token::LBRACKET)) {
        if (!check(Token::RBRACKET)) {
            vd->array_sizes.push_back(parse_expression());
        }
        consume(Token::RBRACKET, "Se esperaba ']' en declaración de arreglo");
    }

    // optional initializer
    if (match(Token::ASSIGN)) {
        vd->initializer = parse_expression();
    }

    consume(Token::SEMICOL, "Se esperaba ';' al final de la declaración");
    return vd;
}

// =============================
// Struct declaration (grammar.md §2)
// =============================

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
// Parameters (grammar.md §4)
// =============================

VarDecl* Parser::parse_parameter() {
    TypeNode* type = parse_type();
    Token* name = consume(Token::ID, "Se esperaba nombre del parámetro");
    VarDecl* param = new VarDecl(type, name->text);

    while (match(Token::LBRACKET)) {
        if (!check(Token::RBRACKET)) {
            param->array_sizes.push_back(parse_expression());
        }
        consume(Token::RBRACKET, "Se esperaba ']'");
    }
    return param;
}

// =============================
// Helper: is_type_start
// =============================

bool Parser::is_type_start() const {
    return check(Token::VOID) || check(Token::INT) || check(Token::CHAR) ||
           check(Token::FLOAT) || check(Token::DOUBLE) || check(Token::BOOL) ||
           check(Token::AUTO) || check(Token::STRUCT);
}

// =============================
// Statements (grammar.md §5)
// =============================

Stm* Parser::parse_statement() {
    if (is_type_start()) {
        return new DeclStmt(parse_local_var_decl());
    }
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
        advance();
        consume(Token::SEMICOL, "Se esperaba ';' después de break");
        return new BreakStmt();
    }
    if (check(Token::CONTINUE)) {
        advance();
        consume(Token::SEMICOL, "Se esperaba ';' después de continue");
        return new ContinueStmt();
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

VarDecl* Parser::parse_local_var_decl() {
    TypeNode* type = parse_type();
    Token* name = consume(Token::ID, "Se esperaba identificador");
    return parse_variable_decl(type, name->text);
}

Stm* Parser::parse_compound_statement() {
    CompoundStmt* cs = new CompoundStmt();
    consume(Token::LBRACE, "Se esperaba '{'");
    while (!check(Token::RBRACE) && !isAtEnd()) {
        cs->stmts.push_back(parse_statement());
    }
    consume(Token::RBRACE, "Se esperaba '}'");
    return cs;
}

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

Stm* Parser::parse_while_statement() {
    consume(Token::WHILE, "Se esperaba 'while'");
    consume(Token::LPAREN, "Se esperaba '(' después de while");
    Exp* cond = parse_expression();
    consume(Token::RPAREN, "Se esperaba ')' después de condición");
    Stm* body = parse_statement();
    return new WhileStmt(cond, body);
}

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

Stm* Parser::parse_for_statement() {
    consume(Token::FOR, "Se esperaba 'for'");
    consume(Token::LPAREN, "Se esperaba '(' después de for");

    Stm* init = nullptr;
    if (!check(Token::SEMICOL)) {
        Exp* e = parse_expression();
        init = new ExprStmtNode(e);
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
            DefaultClause* dc = new DefaultClause();
            while (!check(Token::CASE) && !check(Token::DEFAULT) && !check(Token::RBRACE) && !isAtEnd())
                dc->body.push_back(parse_statement());
            ss->cases.push_back(dc);
        }
    }
    consume(Token::RBRACE, "Se esperaba '}'");
    return ss;
}

Stm* Parser::parse_return_statement() {
    consume(Token::RETURN, "Se esperaba 'return'");
    Exp* expr = nullptr;
    if (!check(Token::SEMICOL)) {
        expr = parse_expression();
    }
    consume(Token::SEMICOL, "Se esperaba ';' después de return");
    return new ReturnStmt(expr);
}

// =============================
// Expressions (grammar.md §6)
// =============================

Exp* Parser::parse_expression() {
    Exp* l = parse_assignment();
    while (match(Token::COMA)) {
        Exp* r = parse_assignment();
        l = new BinaryOpNode(l, r, BinaryOp::COMMA);
    }
    return l;
}

Exp* Parser::parse_assignment() {
    Exp* l = parse_conditional();
    if (match(Token::ASSIGN)) {
        Exp* r = parse_assignment();
        l = new AssignmentNode(l, r, AssignOp::ASSIGN);
    } else if (match(Token::ADD_ASSIGN)) {
        Exp* r = parse_assignment();
        l = new AssignmentNode(l, r, AssignOp::ADD_ASSIGN);
    } else if (match(Token::SUB_ASSIGN)) {
        Exp* r = parse_assignment();
        l = new AssignmentNode(l, r, AssignOp::SUB_ASSIGN);
    } else if (match(Token::MUL_ASSIGN)) {
        Exp* r = parse_assignment();
        l = new AssignmentNode(l, r, AssignOp::MUL_ASSIGN);
    } else if (match(Token::DIV_ASSIGN)) {
        Exp* r = parse_assignment();
        l = new AssignmentNode(l, r, AssignOp::DIV_ASSIGN);
    }
    return l;
}

Exp* Parser::parse_conditional() {
    Exp* cond = parse_logical_or();
    if (match(Token::QUESTION)) {
        Exp* then_expr = parse_expression();
        consume(Token::COLON, "Se esperaba ':' en expresión ternaria");
        Exp* else_expr = parse_conditional();
        cond = new TernaryOpNode(cond, then_expr, else_expr);
    }
    return cond;
}

Exp* Parser::parse_logical_or() {
    Exp* l = parse_logical_and();
    while (match(Token::OR)) {
        Exp* r = parse_logical_and();
        l = new BinaryOpNode(l, r, BinaryOp::LOG_OR);
    }
    return l;
}

Exp* Parser::parse_logical_and() {
    Exp* l = parse_equality();
    while (match(Token::AND)) {
        Exp* r = parse_equality();
        l = new BinaryOpNode(l, r, BinaryOp::LOG_AND);
    }
    return l;
}

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

Exp* Parser::parse_multiplicative() {
    Exp* l = parse_cast();
    while (true) {
        if (match(Token::STAR)) {
            Exp* r = parse_cast();
            l = new BinaryOpNode(l, r, BinaryOp::MUL);
        } else if (match(Token::DIV)) {
            Exp* r = parse_cast();
            l = new BinaryOpNode(l, r, BinaryOp::DIV);
        } else if (match(Token::MOD)) {
            Exp* r = parse_cast();
            l = new BinaryOpNode(l, r, BinaryOp::MOD);
        } else break;
    }
    return l;
}

Exp* Parser::parse_cast() {
    // C-style cast: (type) expr
    // Check by looking at what follows '('
    if (check(Token::LPAREN)) {
        Scanner::Pos saved = scanner->getPos();
        advance(); // consume '('

        bool is_cast = false;
        TypeNode* cast_type = nullptr;

        if (check(Token::VOID) || check(Token::INT) || check(Token::CHAR) ||
            check(Token::FLOAT) || check(Token::DOUBLE) || check(Token::BOOL) ||
            check(Token::AUTO)) {
            // Could be cast like (int)expr
            advance(); // consume type keyword
            if (check(Token::RPAREN)) {
                is_cast = true;
                cast_type = new PrimitiveTypeNode(
                    previous->type == Token::VOID ? PrimitiveTypeNode::VOID :
                    previous->type == Token::INT ? PrimitiveTypeNode::INT :
                    previous->type == Token::CHAR ? PrimitiveTypeNode::CHAR :
                    previous->type == Token::FLOAT ? PrimitiveTypeNode::FLOAT :
                    previous->type == Token::DOUBLE ? PrimitiveTypeNode::DOUBLE :
                    previous->type == Token::BOOL ? PrimitiveTypeNode::BOOL :
                    PrimitiveTypeNode::AUTO);
                advance(); // consume ')'
            }
        } else if (check(Token::STRUCT)) {
            // Possibly struct type cast: (struct id) expr
            advance(); // consume STRUCT
            if (check(Token::ID)) {
                string sname = current->text;
                advance(); // consume ID
                if (check(Token::RPAREN)) {
                    is_cast = true;
                    cast_type = new StructTypeNode(sname);
                    advance(); // consume ')'
                }
            }
        }

        if (is_cast) {
            Exp* operand = parse_cast();
            return new CastNode(cast_type, operand);
        }

        // Not a cast — parse as parenthesized expression.
        // current already points to the first token after '('
        Exp* expr = parse_expression();
        consume(Token::RPAREN, "Se esperaba ')'");
        return new ParenthesizedExprNode(expr);
    }
    return parse_unary();
}

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
        Exp* operand = parse_cast();
        return new UnaryOpNode(operand, UnaryOp::ADDR);
    }
    if (match(Token::STAR)) {
        Exp* operand = parse_cast();
        return new UnaryOpNode(operand, UnaryOp::DEREF);
    }
    if (match(Token::MINUS)) {
        Exp* operand = parse_cast();
        return new UnaryOpNode(operand, UnaryOp::MINUS);
    }
    if (match(Token::NOT)) {
        Exp* operand = parse_cast();
        return new UnaryOpNode(operand, UnaryOp::LOG_NOT);
    }
    return parse_postfix();
}

Exp* Parser::parse_postfix() {
    Exp* l = parse_primary();

    while (true) {
        if (match(Token::LBRACKET)) {
            Exp* index = parse_expression();
            consume(Token::RBRACKET, "Se esperaba ']'");
            l = new SubscriptNode(l, index);
        } else if (match(Token::LPAREN)) {
            CallNode* call = new CallNode(l);
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
        string text = previous->text;
        if (text.find('.') != string::npos || text.find('e') != string::npos || text.find('E') != string::npos) {
            return new FloatLiteralNode(stod(text));
        }
        return new IntegerLiteralNode(stoll(text));
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
        CompoundStmt* lbody = dynamic_cast<CompoundStmt*>(parse_compound_statement());
        if (!lbody) sync_error("Se esperaba cuerpo compuesto en lambda");
        return new LambdaExprNode(captures, lparams, lret, lbody);
    }
    if (match(Token::LPAREN)) {
        Exp* expr = parse_expression();
        consume(Token::RPAREN, "Se esperaba ')'");
        return new ParenthesizedExprNode(expr);
    }
    if (match(Token::MALLOC)) {
        consume(Token::LPAREN, "Se esperaba '(' después de malloc");
        Exp* size = parse_expression();
        consume(Token::RPAREN, "Se esperaba ')'");
        return new MallocNode(size);
    }
    sync_error("Se esperaba una expresión");
    return nullptr;
}
