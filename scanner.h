#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include "token.h"

using namespace std;

class Scanner {
private:
    string input;
    int first;
    int current;
    int line, column; // index de scanner
    int first_line, first_column; // inicio de token

    char peek() const;
    char advance();
    bool match_advance(char expected);
    void skip_whitespace();
    Token* make_token(Token::Type type) const;
    Token* make_token(Token::Type type, char c) const;
    Token* identifier_or_keyword();
    Token* number();
    Token* char_literal();
    Token* string_literal();

public:
    struct Pos {
        int first, current;
        int line, column;
    };

    Scanner(const char* in_s);
    Token* nextToken();
    Pos getPos() const;
    void setPos(Pos p);
    ~Scanner();
};

void ejecutar_scanner(Scanner* scanner, const string& InputFile);

#endif
