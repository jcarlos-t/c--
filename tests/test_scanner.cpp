#include <iostream>
#include <cassert>
#include "scanner.h"

using namespace std;

#define T_ASSERT(cond) do { if (!(cond)) { cerr << "FAIL: " #cond << " (line " << __LINE__ << ")" << endl; exit(1); } } while(0)

void check_tok(Token* t, Token::Type ex_type, int ex_line, int ex_col, const string& ex_text) {
    T_ASSERT(t != nullptr);
    T_ASSERT(t->type == ex_type);
    T_ASSERT(t->line == ex_line);
    T_ASSERT(t->col == ex_col);
    T_ASSERT(t->text == ex_text);
    delete t;
}

void check_type(Token* t, Token::Type ex_type) {
    T_ASSERT(t != nullptr);
    T_ASSERT(t->type == ex_type);
    delete t;
}

int passes = 0, fails = 0;

void test(const string& name) {
    cout << "  " << name << "... ";
}

void pass() {
    cout << "PASS" << endl;
    passes++;
}

void fail(const string& msg) {
    cout << "FAIL: " << msg << endl;
    fails++;
}

void test_keywords_and_types() {
    test("keywords_and_types");
    Scanner sc("void int char float double bool auto");
    check_tok(sc.nextToken(), Token::VOID, 1, 1, "void");
    check_tok(sc.nextToken(), Token::INT, 1, 6, "int");
    check_tok(sc.nextToken(), Token::CHAR, 1, 10, "char");
    check_tok(sc.nextToken(), Token::FLOAT, 1, 15, "float");
    check_tok(sc.nextToken(), Token::DOUBLE, 1, 21, "double");
    check_tok(sc.nextToken(), Token::BOOL, 1, 28, "bool");
    check_tok(sc.nextToken(), Token::AUTO, 1, 33, "auto");
    check_type(sc.nextToken(), Token::END);
    pass();
}

void test_control_flow() {
    test("control_flow");
    Scanner sc("if else while do for switch case default break continue return");
    check_tok(sc.nextToken(), Token::IF, 1, 1, "if");
    check_tok(sc.nextToken(), Token::ELSE, 1, 4, "else");
    check_tok(sc.nextToken(), Token::WHILE, 1, 9, "while");
    check_tok(sc.nextToken(), Token::DO, 1, 15, "do");
    check_tok(sc.nextToken(), Token::FOR, 1, 18, "for");
    check_tok(sc.nextToken(), Token::SWITCH, 1, 22, "switch");
    check_tok(sc.nextToken(), Token::CASE, 1, 29, "case");
    check_tok(sc.nextToken(), Token::DEFAULT, 1, 34, "default");
    check_tok(sc.nextToken(), Token::BREAK, 1, 42, "break");
    check_tok(sc.nextToken(), Token::CONTINUE, 1, 48, "continue");
    check_tok(sc.nextToken(), Token::RETURN, 1, 57, "return");
    check_type(sc.nextToken(), Token::END);
    pass();
}

void test_operators() {
    test("operators");
    Scanner sc("+ - * / % ** = == != < > <= >= && || ! & -> . ++ --");
    check_tok(sc.nextToken(), Token::PLUS, 1, 1, "+");
    check_tok(sc.nextToken(), Token::MINUS, 1, 3, "-");
    check_tok(sc.nextToken(), Token::STAR, 1, 5, "*");
    check_tok(sc.nextToken(), Token::DIV, 1, 7, "/");
    check_tok(sc.nextToken(), Token::MOD, 1, 9, "%");
    check_tok(sc.nextToken(), Token::POW, 1, 11, "**");
    check_tok(sc.nextToken(), Token::ASSIGN, 1, 14, "=");
    check_tok(sc.nextToken(), Token::EQ, 1, 16, "==");
    check_tok(sc.nextToken(), Token::NE, 1, 19, "!=");
    check_tok(sc.nextToken(), Token::LT, 1, 22, "<");
    check_tok(sc.nextToken(), Token::GT, 1, 24, ">");
    check_tok(sc.nextToken(), Token::LE, 1, 26, "<=");
    check_tok(sc.nextToken(), Token::GE, 1, 29, ">=");
    check_tok(sc.nextToken(), Token::AND, 1, 32, "&&");
    check_tok(sc.nextToken(), Token::OR, 1, 35, "||");
    check_tok(sc.nextToken(), Token::NOT, 1, 38, "!");
    check_tok(sc.nextToken(), Token::AMPERSAND, 1, 40, "&");
    check_tok(sc.nextToken(), Token::ARROW, 1, 42, "->");
    check_tok(sc.nextToken(), Token::DOT, 1, 45, ".");
    check_tok(sc.nextToken(), Token::INC, 1, 47, "++");
    check_tok(sc.nextToken(), Token::DEC, 1, 50, "--");
    check_type(sc.nextToken(), Token::END);
    pass();
}

void test_line_column() {
    test("line_column");
    Scanner sc("int x;\nint y;\nfloat z;");
    check_tok(sc.nextToken(), Token::INT, 1, 1, "int");
    check_tok(sc.nextToken(), Token::ID, 1, 5, "x");
    check_tok(sc.nextToken(), Token::SEMICOL, 1, 6, ";");
    check_tok(sc.nextToken(), Token::INT, 2, 1, "int");
    check_tok(sc.nextToken(), Token::ID, 2, 5, "y");
    check_tok(sc.nextToken(), Token::SEMICOL, 2, 6, ";");
    check_tok(sc.nextToken(), Token::FLOAT, 3, 1, "float");
    check_tok(sc.nextToken(), Token::ID, 3, 7, "z");
    check_tok(sc.nextToken(), Token::SEMICOL, 3, 8, ";");
    check_type(sc.nextToken(), Token::END);
    pass();
}

void test_error_token() {
    test("error_token");
    Scanner sc("@ |");
    Token* t1 = sc.nextToken();
    T_ASSERT(t1->type == Token::ERR);
    T_ASSERT(t1->line == 1 && t1->col == 1);
    delete t1;
    Token* t2 = sc.nextToken();
    T_ASSERT(t2->type == Token::ERR);
    T_ASSERT(t2->line == 1 && t2->col == 3);
    delete t2;
    pass();
}

void test_unclosed_string() {
    test("unclosed_string");
    Scanner sc("\"hello");
    Token* t = sc.nextToken();
    T_ASSERT(t->type == Token::ERR);
    delete t;
    pass();
}

void test_pow_token() {
    test("pow_token");
    Scanner sc("2 ** 3");
    check_tok(sc.nextToken(), Token::NUM, 1, 1, "2");
    check_tok(sc.nextToken(), Token::POW, 1, 3, "**");
    check_tok(sc.nextToken(), Token::NUM, 1, 6, "3");
    check_type(sc.nextToken(), Token::END);
    pass();
}

int main() {
    test_keywords_and_types();
    test_control_flow();
    test_operators();
    test_line_column();
    test_error_token();
    test_unclosed_string();
    test_pow_token();
    cout << "\n=== Scanner: " << passes << " passed, " << fails << " failed ===" << endl;
    return fails > 0 ? 1 : 0;
}
