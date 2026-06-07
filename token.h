#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <ostream>

using namespace std;

class Token {
public:
    // Tipos de token
    enum Type {
        PLUS,    // +
        MINUS,   // -
        MUL,     // *
        DIV,     // /
        POW,     // **
        LPAREN,  // (
        RPAREN,  // )
        NUM,     // Número
        ERR,     // Error
        ID,      // ID
        FUN,
        ENDFUN,
        COMA,
        RETURN,
        TRUE,
        FALSE,
        VAR,
        LE,
        AND,
        SEMICOL,    
        ASSIGN,
        COLON,
        PRINT,
        WHILE,
        ENDWHILE,
        IF,
        ELSE,
        ENDIF,
        END      // Fin de entrada
    };

    // Atributos
    Type type;
    string text;

    // Constructores
    Token(Type type);
    Token(Type type, char c);
    Token(Type type, const string& source, int first, int last);

    // Sobrecarga de operadores de salida
    friend ostream& operator<<(ostream& outs, const Token& tok);
    friend ostream& operator<<(ostream& outs, const Token* tok);
};

#endif // TOKEN_H