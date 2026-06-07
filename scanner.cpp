#include <iostream>
#include <cstring>
#include <cctype>
#include <fstream>
#include "token.h"
#include "scanner.h"

using namespace std;

// -----------------------------
// Constructor
// -----------------------------
Scanner::Scanner(const char* s): input(s), first(0), current(0) { 
    }

// -----------------------------
// Función auxiliar
// -----------------------------

bool is_white_space(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

// -----------------------------
// nextToken: obtiene el siguiente token
// -----------------------------


Token* Scanner::nextToken() {
    Token* token;

    // Saltar espacios en blanco
    while (current < input.length() && is_white_space(input[current])) 
        current++;

    // Fin de la entrada
    if (current >= input.length()) 
        return new Token(Token::END);

    char c = input[current];

    first = current;

    // Números (enteros y flotantes)
    if (isdigit(c)) {
        current++;
        while (current < input.length() && isdigit(input[current]))
            current++;
        if (current < input.length() && input[current] == '.') {
            current++;
            while (current < input.length() && isdigit(input[current]))
                current++;
        }
        token = new Token(Token::NUM, input, first, current - first);
    }
    // ID
    else if (isalpha(c)) {
        current++;
        while (current < input.length() && isalnum(input[current]))
            current++;
        string lexema = input.substr(first, current - first);
        // Case-insensitive keyword matching
        string lower;
        for (char ch : lexema) lower += tolower(ch);
        if (lower=="print") return new Token(Token::PRINT, input, first, current - first);
        else if (lower=="return") return new Token(Token::RETURN, input, first, current - first);
        else if (lower=="var") return new Token(Token::VAR, input, first, current - first);
        else if (lower=="fun") return new Token(Token::FUN, input, first, current - first);                
        else if (lower=="endfun") return new Token(Token::ENDFUN, input, first, current - first);
        else if (lower=="false") return new Token(Token::FALSE, input, first, current - first);
        else if (lower=="true") return new Token(Token::TRUE, input, first, current - first);
        else if (lower=="and") return new Token(Token::AND, input, first, current - first);
        else if (lower=="while") return new Token(Token::WHILE, input, first, current - first);
        else if (lower=="endwhile") return new Token(Token::ENDWHILE, input, first, current - first);
        else if (lower=="if") return new Token(Token::IF, input, first, current - first);
        else if (lower=="else") return new Token(Token::ELSE, input, first, current - first);
        else if (lower=="endif") return new Token(Token::ENDIF, input, first, current - first);
        else return new Token(Token::ID, input, first, current - first);
    }
    // Operadores
    else if (strchr("+/-*();=,<:", c)) {
        switch (c) {
            case ',': token = new Token(Token::COMA,  c); break;
            case '<': token = new Token(Token::LE, c); break;
            case '+': token = new Token(Token::PLUS,  c); break;
            case '-': token = new Token(Token::MINUS, c); break;
            case '*': 
            if (input[current+1]=='*')
            {
                current++;
                token = new Token(Token::POW, input, first, current + 1 - first);
            }
            else{
                token = new Token(Token::MUL,   c);
            }
            break;
            case '/': token = new Token(Token::DIV,   c); break;
            case '(': token = new Token(Token::LPAREN,c); break;
            case ')': token = new Token(Token::RPAREN,c); break;
            case '=': token = new Token(Token::ASSIGN,c); break;
            case ';': token = new Token(Token::SEMICOL,c); break;
            case ':': token = new Token(Token::COLON,c); break;
        }
        current++;
    }

    // Carácter inválido
    else {
        token = new Token(Token::ERR, c);
        current++;
    }

    return token;
}




// -----------------------------
// Destructor
// -----------------------------
Scanner::~Scanner() { }

// -----------------------------
// Función de prueba
// -----------------------------

void ejecutar_scanner(Scanner* scanner, const string& InputFile) {
    Token* tok;

    // Crear nombre para archivo de salida
    string OutputFileName = InputFile;
    size_t pos = OutputFileName.find_last_of(".");
    if (pos != string::npos) {
        OutputFileName = OutputFileName.substr(0, pos);
    }
    OutputFileName += "_tokens.txt";

    ofstream outFile(OutputFileName);
    if (!outFile.is_open()) {
        cerr << "Error: no se pudo abrir el archivo " << OutputFileName << endl;
        return;
    }

    outFile << "Scanner\n" << endl;

    while (true) {
        tok = scanner->nextToken();

        if (tok->type == Token::END) {
            outFile << *tok << endl;
            delete tok;
            outFile << "\nScanner exitoso" << endl << endl;
            outFile.close();
            return;
        }

        if (tok->type == Token::ERR) {
            outFile << *tok << endl;
            delete tok;
            outFile << "Caracter invalido" << endl << endl;
            outFile << "Scanner no exitoso" << endl << endl;
            outFile.close();
            return;
        }

        outFile << *tok << endl;
        delete tok;
    }
}
