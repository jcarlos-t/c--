#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <ostream>

using namespace std;

class Token {
public:
    enum Type {
        // Tipos de dato
        VOID, INT, CHAR, FLOAT, DOUBLE, BOOL, LONG,

        // Palabras clave de control y declaración
        STRUCT, IF, ELSE, WHILE, DO, FOR, SWITCH,
        CASE, DEFAULT, BREAK, CONTINUE, RETURN,
        SIZEOF, MALLOC, FREE, CONST, UNSIGNED,

        // Puntuación y delimitadores
        LPAREN, RPAREN, LBRACE, RBRACE,
        LBRACKET, RBRACKET, SEMICOL, COMA, COLON, QUESTION,

        // Operadores aritméticos
        PLUS, MINUS, STAR, DIV, MOD, POW,

        // Operadores de asignación
        ASSIGN,

        // Operadores de comparación
        EQ, NE, LT, GT, LE, GE,

        // Operadores lógicos
        AND, OR, NOT,

        // Incremento y decremento
        INC, DEC,

        // Otros operadores
        AMPERSAND, ARROW, DOT,

        // Literales
        NUM, FNUM, CHAR_LIT, STRING_LIT,

        // Booleanos
        TRUE, FALSE,

        // Otros
        PRINTF,
        ID, END, ERR
    };

    enum class LiteralSuffix {
        SUF_NONE,
        SUF_F,
        SUF_L,
        SUF_LL,
        SUF_U,
        SUF_UL,
        SUF_ULL
    };

    Type type;
    string text;
    int line;
    int col;
    LiteralSuffix suffix = LiteralSuffix::SUF_NONE;

    Token(Type type);
    Token(Type type, char c);
    Token(Type type, const string& source, int first, int last);

    friend ostream& operator<<(ostream& outs, const Token& tok);
    friend ostream& operator<<(ostream& outs, const Token* tok);
};

const char* token_name(Token::Type t);
const char* token_type_to_string(Token::Type t);

#endif
