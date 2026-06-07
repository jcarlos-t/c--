#include <iostream>
#include <cstring>
#include <cctype>
#include <fstream>
#include "token.h"
#include "scanner.h"

using namespace std;

Scanner::Scanner(const char* s) : input(s), first(0), current(0) {}

Scanner::~Scanner() {}

Scanner::Pos Scanner::getPos() const {
    return {first, current};
}

void Scanner::setPos(Pos p) {
    first = p.first;
    current = p.current;
}

char Scanner::peek() const {
    if (current >= (int)input.length()) return '\0';
    return input[current];
}

char Scanner::advance() {
    return input[current++];
}

bool Scanner::match_advance(char expected) {
    if (peek() != expected) return false;
    current++;
    return true;
}

void Scanner::skip_whitespace() {
    while (current < (int)input.length()) {
        char c = input[current];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
            current++;
        else if (c == '/' && current + 1 < (int)input.length() && input[current + 1] == '/') {
            while (current < (int)input.length() && input[current] != '\n')
                current++;
        }
        else if (c == '/' && current + 1 < (int)input.length() && input[current + 1] == '*') {
            current += 2;
            while (current + 1 < (int)input.length() && !(input[current] == '*' && input[current + 1] == '/'))
                current++;
            if (current + 1 < (int)input.length()) current += 2;
        }
        else
            break;
    }
}

Token* Scanner::make_token(Token::Type type) const {
    return new Token(type, input, first, current - first);
}

Token* Scanner::make_token(Token::Type type, char c) const {
    return new Token(type, c);
}

Token* Scanner::identifier_or_keyword() {
    while (current < (int)input.length() && (isalnum(peek()) || peek() == '_'))
        advance();
    string word = input.substr(first, current - first);

    if (word == "void")     return make_token(Token::VOID);
    if (word == "int")      return make_token(Token::INT);
    if (word == "char")     return make_token(Token::CHAR);
    if (word == "float")    return make_token(Token::FLOAT);
    if (word == "double")   return make_token(Token::DOUBLE);
    if (word == "bool")     return make_token(Token::BOOL);
    if (word == "true")     return make_token(Token::TRUE);
    if (word == "false")    return make_token(Token::FALSE);
    if (word == "auto")     return make_token(Token::AUTO);
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
    if (word == "template") return make_token(Token::TEMPLATE);
    if (word == "typename") return make_token(Token::TYPENAME);

    return make_token(Token::ID);
}

Token* Scanner::number() {
    while (current < (int)input.length() && isdigit(peek()))
        advance();

    if (peek() == '.') {
        advance();
        while (current < (int)input.length() && isdigit(peek()))
            advance();
    }

    if (peek() == 'e' || peek() == 'E') {
        advance();
        if (peek() == '+' || peek() == '-') advance();
        while (current < (int)input.length() && isdigit(peek()))
            advance();
    }

    return make_token(Token::NUM);
}

Token* Scanner::char_literal() {
    advance(); // skip opening '
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

Token* Scanner::string_literal() {
    advance(); // skip opening "
    while (current < (int)input.length() && peek() != '"') {
        if (peek() == '\\') {
            advance();
            if (current < (int)input.length()) advance();
        } else {
            advance();
        }
    }
    if (current < (int)input.length())
        advance(); // skip closing "
    else
        return new Token(Token::ERR, input, first, current - first);
    return make_token(Token::STRING_LIT);
}

Token* Scanner::nextToken() {
    skip_whitespace();

    if (current >= (int)input.length())
        return new Token(Token::END);

    first = current;
    char c = advance();

    if (isalpha(c) || c == '_')
        return identifier_or_keyword();

    if (isdigit(c)) {
        current--; // back up, number() will re-read
        return number();
    }

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
            if (peek() == '=') { advance(); return make_token(Token::ADD_ASSIGN); }
            return make_token(Token::PLUS, c);

        case '-':
            if (peek() == '-') { advance(); return make_token(Token::DEC); }
            if (peek() == '=') { advance(); return make_token(Token::SUB_ASSIGN); }
            if (peek() == '>') { advance(); return make_token(Token::ARROW); }
            return make_token(Token::MINUS, c);

        case '*':
            if (peek() == '*') { advance(); return make_token(Token::POW); }
            if (peek() == '=') { advance(); return make_token(Token::MUL_ASSIGN); }
            return make_token(Token::STAR, c);

        case '/':
            if (peek() == '=') { advance(); return make_token(Token::DIV_ASSIGN); }
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
            return new Token(Token::ERR, c);

        default:
            return new Token(Token::ERR, c);
    }
}

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
            outFile << "Caracter invalido" << endl << endl;
            outFile << "Scanner no exitoso" << endl << endl;
            outFile.close();
            return;
        }
        outFile << *tok << endl;
        delete tok;
    }
}
