#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"
#include "ast.h"
#include <unordered_set>

class Parser {
private:
    Scanner* scanner;
    Token *current, *previous;
    int current_line, current_column;

    bool match(Token::Type ttype);
    bool check(Token::Type ttype) const;
    bool advance();
    bool isAtEnd() const;
    Token* consume(Token::Type ttype, const string& msg);
    void sync_error(const string& msg);

    // Types
    TypeNode* parse_type();
    TypeNode* parse_basic_type();

    // Declarations
    Program* parse_program();
    void parse_declaration(Program* p);
    FunDecl* parse_function_decl(Exp* ret_type, const string& name);
    VarDecl* parse_variable_decl(Exp* type, const string& name);
    StructDecl* parse_struct_decl();

    // Parameters
    VarDecl* parse_parameter();

    // Statements
    Stm* parse_statement();
    Stm* parse_compound_statement();
    Stm* parse_if_statement();
    Stm* parse_while_statement();
    Stm* parse_do_while_statement();
    Stm* parse_for_statement();
    Stm* parse_switch_statement();
    Stm* parse_return_statement();

    // Expressions (precedence climbing per grammar.md §6)
    Exp* parse_expression();
    Exp* parse_assignment();
    Exp* parse_conditional();
    Exp* parse_logical_or();
    Exp* parse_logical_and();
    Exp* parse_equality();
    Exp* parse_relational();
    Exp* parse_additive();
    Exp* parse_multiplicative();
    Exp* parse_cast();
    Exp* parse_unary();
    Exp* parse_postfix();
    Exp* parse_primary();
    Exp* parse_constant();

    // Helpers
    Exp* parse_argument_list();
    bool is_type_start() const;
    VarDecl* parse_local_var_decl();

    unordered_set<string> struct_names;

public:
    Parser(Scanner* scanner);
    Program* parseProgram();
};

#endif
