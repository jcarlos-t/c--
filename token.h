#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <ostream>

using namespace std;

class Token {
public:
    enum Type {
        // Types
        VOID, INT, CHAR, FLOAT, DOUBLE, BOOL, AUTO,

        // Keywords
        STRUCT, IF, ELSE, WHILE, DO, FOR, SWITCH,
        CASE, DEFAULT, BREAK, CONTINUE, RETURN,
        SIZEOF, MALLOC, FREE, TEMPLATE, TYPENAME,

        // Punctuation
        LPAREN, RPAREN, LBRACE, RBRACE,
        LBRACKET, RBRACKET, SEMICOL, COMA, COLON, QUESTION,

        // Arithmetic operators
        PLUS, MINUS, STAR, DIV, MOD, POW,

        // Assignment
        ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN,

        // Comparison
        EQ, NE, LT, GT, LE, GE,

        // Logical
        AND, OR, NOT,

        // Inc/Dec
        INC, DEC,

        // Other operators
        AMPERSAND, ARROW, DOT,

        // Literals
        NUM, CHAR_LIT, STRING_LIT,

        // Literals
        TRUE, FALSE,

        // Other
        ID, END, ERR
    };

    Type type;
    string text;

    Token(Type type);
    Token(Type type, char c);
    Token(Type type, const string& source, int first, int last);

    friend ostream& operator<<(ostream& outs, const Token& tok);
    friend ostream& operator<<(ostream& outs, const Token* tok);
};

#endif
