#include <iostream>
#include <cstring>
#include <cctype>
#include <fstream>
#include "token.h"
#include "scanner.h"

using namespace std;

// Constructor: inicializa scanner con el string de entrada
Scanner::Scanner(const char* s) : input(s), first(0), current(0), line(1), column(1), first_line(1), first_column(1) {}

Scanner::~Scanner() {}

// Retorna la posición actual (para retroceder en el parsing)
Scanner::Pos Scanner::getPos() const {
    return {first, current, line, column};
}

// Restaura una posición previamente guardada
void Scanner::setPos(Pos p) {
    first = p.first;
    current = p.current;
    line = p.line;
    column = p.column;
}

// Retorna el caracter actual sin consumirlo
char Scanner::peek() const {
    if (current >= (int)input.length()) return '\0';
    return input[current];
}

// Consume y retorna el siguiente caracter, actualizando línea/columna
char Scanner::advance() {
    char c = input[current++];
    if (c == '\n') { line++; column = 1; }
    else column++;
    return c;
}

// Si el siguiente caracter coincide, lo consume y retorna true
bool Scanner::match_advance(char expected) {
    if (peek() != expected) return false;
    current++;
    column++;
    return true;
}

// Salta espacios, tabs, saltos de línea y comentarios // /* */
void Scanner::skip_whitespace() {
    while (current < (int)input.length()) {
        char c = input[current];
        if (c == ' ' || c == '\r' || c == '\t') {
            current++;
            column++;
        }
        else if (c == '\n') {
            current++;
            line++;
            column = 1;
        }
        else if (c == '/' && current + 1 < (int)input.length() && input[current + 1] == '/') {
            while (current < (int)input.length() && input[current] != '\n')
                current++;
        }
        else if (c == '/' && current + 1 < (int)input.length() && input[current + 1] == '*') {
            current += 2;
            column += 2;
            while (current + 1 < (int)input.length() && !(input[current] == '*' && input[current + 1] == '/')) {
                if (input[current] == '\n') { line++; column = 1; }
                else column++;
                current++;
            }
            if (current + 1 < (int)input.length()) current += 2;
        }
        else
            break;
    }
}

// Crea un token con el lexema desde first hasta current
Token* Scanner::make_token(Token::Type type) const {
    Token* t = new Token(type, input, first, current - first);
    t->line = first_line;
    t->col = first_column;
    return t;
}

// Crea un token de un solo caracter
Token* Scanner::make_token(Token::Type type, char c) const {
    Token* t = new Token(type, c);
    t->line = first_line;
    t->col = first_column;
    return t;
}

// Reconoce identificadores y palabras clave, retorna el token correspondiente
Token* Scanner::identifier_or_keyword() {
    while (current < (int)input.length() && (isalnum(peek()) || peek() == '_'))
        advance();
    string word = input.substr(first, current - first);

    // tipos base
    if (word == "void")     return make_token(Token::VOID);
    if (word == "int")      return make_token(Token::INT);
    if (word == "char")     return make_token(Token::CHAR);
    if (word == "float")    return make_token(Token::FLOAT);
    if (word == "double")   return make_token(Token::DOUBLE);
    if (word == "bool")     return make_token(Token::BOOL);
    if (word == "long")     return make_token(Token::LONG);
    
    // booleanos, estructuras de control
    if (word == "true")     return make_token(Token::TRUE);
    if (word == "false")    return make_token(Token::FALSE);
    if (word == "struct")   return make_token(Token::STRUCT);
    if (word == "if")       return make_token(Token::IF);
    if (word == "else")     return make_token(Token::ELSE);
    if (word == "while")    return make_token(Token::WHILE);
    if (word == "do")       return make_token(Token::DO);
    if (word == "for")      return make_token(Token::FOR);
    if (word == "switch")   return make_token(Token::SWITCH);
    if (word == "case")     return make_token(Token::CASE);
    if (word == "default")  return make_token(Token::DEFAULT);
    if (word == "break")    return make_token(Token::BREAK);
    if (word == "continue") return make_token(Token::CONTINUE);
    if (word == "return")   return make_token(Token::RETURN);
    if (word == "sizeof")   return make_token(Token::SIZEOF);
    if (word == "malloc")   return make_token(Token::MALLOC);
    if (word == "free")     return make_token(Token::FREE);
    if (word == "const")    return make_token(Token::CONST);
    if (word == "unsigned") return make_token(Token::UNSIGNED);

    if (word == "printf") return make_token(Token::PRINTF);

    return make_token(Token::ID);
}

// Reconoce literales numéricos: enteros y flotantes
Token* Scanner::number() {
    bool is_float = false;
    while (current < (int)input.length() && isdigit(peek()))
        advance();

    if (peek() == '.') {
        is_float = true;
        advance();
        while (current < (int)input.length() && isdigit(peek()))
            advance();
    }

    if (peek() == 'e' || peek() == 'E') {
        is_float = true;
        advance();
        if (peek() == '+' || peek() == '-') advance();
        while (current < (int)input.length() && isdigit(peek()))
            advance();
    }

    // Sufijo ll/LL para long long
    if (!is_float && (peek() == 'l' || peek() == 'L')) {
        if (current + 1 < (int)input.length() && input[current + 1] == (peek() == 'l' ? 'l' : 'L'))
            current += 2;
    }

    return make_token(is_float ? Token::FNUM : Token::NUM);
}

// Reconoce literales de caracter: 'a', '\n'
Token* Scanner::char_literal() {
    // nextToken() ya consumió la comilla de apertura
    if (current < (int)input.length() && peek() == '\\') {
        advance();
        if (current < (int)input.length()) advance();
    } else {
        if (current < (int)input.length()) advance();
    }
    if (current < (int)input.length() && peek() == '\'')
        advance();
    else {
        return new Token(Token::ERR, input, first, current - first);
    }
    return make_token(Token::CHAR_LIT);
}

// Reconoce literales de cadena: "texto", con escapes
Token* Scanner::string_literal() {
    advance(); // salta la comilla de apertura
    while (current < (int)input.length() && peek() != '"') {
        if (peek() == '\\') {
            advance();
            if (current < (int)input.length()) advance();
        } else {
            advance();
        }
    }
    if (current < (int)input.length())
        advance(); // salta la comilla de cierre
    else
        return new Token(Token::ERR, input, first, current - first);
    return make_token(Token::STRING_LIT);
}

// Función principal: retorna el siguiente token del fuente
Token* Scanner::nextToken() {
    // 1. saltar espacios y comentarios
    skip_whitespace();

    // 2. fin de archivo
    if (current >= (int)input.length()) {
        Token* t = new Token(Token::END);
        t->line = line; t->col = column;
        return t;
    }

    // 3. marcar inicio del lexema
    first = current;
    first_line = line;
    first_column = column;
    char c = advance();

    // 4. identificador o palabra clave
    if (isalpha(c) || c == '_')
        return identifier_or_keyword();

    // 5. número (entero o flotante)
    if (isdigit(c)) {
        current--;
        column--;
        return number();
    }

    // 6. símbolos y operadores de uno o dos caracteres
    switch (c) {
        case '\'': return char_literal();
        case '"':  return string_literal();

        case '(': return make_token(Token::LPAREN, c);
        case ')': return make_token(Token::RPAREN, c);
        case '{': return make_token(Token::LBRACE, c);
        case '}': return make_token(Token::RBRACE, c);
        case '[': return make_token(Token::LBRACKET, c);
        case ']': return make_token(Token::RBRACKET, c);
        case ';': return make_token(Token::SEMICOL, c);
        case ',': return make_token(Token::COMA, c);
        case '?': return make_token(Token::QUESTION, c);
        case '.': return make_token(Token::DOT, c);
        case ':': return make_token(Token::COLON, c);

        case '+':
            if (peek() == '+') { advance(); return make_token(Token::INC); }
            return make_token(Token::PLUS, c);

        case '-':
            if (peek() == '-') { advance(); return make_token(Token::DEC); }
            if (peek() == '>') { advance(); return make_token(Token::ARROW); }
            return make_token(Token::MINUS, c);

        case '*':
            if (peek() == '*') { advance(); return make_token(Token::POW); }
            return make_token(Token::STAR, c);

        case '/':
            return make_token(Token::DIV, c);

        case '%': return make_token(Token::MOD, c);

        case '=':
            if (peek() == '=') { advance(); return make_token(Token::EQ); }
            return make_token(Token::ASSIGN, c);

        case '!':
            if (peek() == '=') { advance(); return make_token(Token::NE); }
            return make_token(Token::NOT, c);

        case '<':
            if (peek() == '=') { advance(); return make_token(Token::LE); }
            return make_token(Token::LT, c);

        case '>':
            if (peek() == '=') { advance(); return make_token(Token::GE); }
            return make_token(Token::GT, c);

        case '&':
            if (peek() == '&') { advance(); return make_token(Token::AND); }
            return make_token(Token::AMPERSAND, c);

        case '|':
            if (peek() == '|') { advance(); return make_token(Token::OR); }
            { Token* t = new Token(Token::ERR, c); t->line = first_line; t->col = first_column; return t; }

        default:
            { Token* t = new Token(Token::ERR, c); t->line = first_line; t->col = first_column; return t; }
    }
}

// Escanea un archivo completo y escribe los resultados a un .txt
void ejecutar_scanner(Scanner* scanner, const string& InputFile) {
    Token* tok;

    string OutputFileName = InputFile;
    size_t pos = OutputFileName.find_last_of(".");
    if (pos != string::npos)
        OutputFileName = OutputFileName.substr(0, pos);
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
            outFile << "Caracter invalido en linea " << tok->line << ":" << tok->col << endl << endl;
            outFile << "Scanner no exitoso" << endl << endl;
            outFile.close();
            return;
        }
        outFile << *tok << endl;
        delete tok;
    }
}
