#include <iostream>
#include "token.h"

using namespace std;

Token::Token(Type type)
    : type(type), text(""), line(0), col(0) { }

Token::Token(Type type, char c)
    : type(type), text(string(1, c)), line(0), col(0) { }

Token::Token(Type type, const string& source, int first, int last)
    : type(type), text(source.substr(first, last)), line(0), col(0) { }

const char* token_name(Token::Type t) {
    switch (t) {
        case Token::VOID:   return "VOID";
        case Token::INT:    return "INT";
        case Token::CHAR:   return "CHAR";
        case Token::FLOAT:  return "FLOAT";
        case Token::DOUBLE: return "DOUBLE";
        case Token::BOOL:   return "BOOL";
        case Token::LONG:   return "LONG";
        case Token::STRUCT:   return "STRUCT";
        case Token::IF:     return "IF";
        case Token::ELSE:   return "ELSE";
        case Token::WHILE:  return "WHILE";
        case Token::DO:     return "DO";
        case Token::FOR:    return "FOR";
        case Token::SWITCH: return "SWITCH";
        case Token::CASE:   return "CASE";
        case Token::DEFAULT:return "DEFAULT";
        case Token::BREAK:  return "BREAK";
        case Token::CONTINUE:return "CONTINUE";
        case Token::RETURN: return "RETURN";
        case Token::SIZEOF: return "SIZEOF";
        case Token::MALLOC: return "MALLOC";
        case Token::FREE:   return "FREE";
        case Token::CONST:   return "CONST";
        case Token::UNSIGNED:return "UNSIGNED";
        case Token::LPAREN: return "LPAREN";
        case Token::RPAREN: return "RPAREN";
        case Token::LBRACE: return "LBRACE";
        case Token::RBRACE: return "RBRACE";
        case Token::LBRACKET:return "LBRACKET";
        case Token::RBRACKET:return "RBRACKET";
        case Token::SEMICOL:return "SEMICOL";
        case Token::COMA:   return "COMA";
        case Token::COLON:  return "COLON";
        case Token::QUESTION:return "QUESTION";
        case Token::PLUS:   return "PLUS";
        case Token::MINUS:  return "MINUS";
        case Token::STAR:   return "STAR";
        case Token::DIV:    return "DIV";
        case Token::MOD:    return "MOD";
        case Token::POW:    return "POW";
        case Token::ASSIGN: return "ASSIGN";

        case Token::EQ:     return "EQ";
        case Token::NE:     return "NE";
        case Token::LT:     return "LT";
        case Token::GT:     return "GT";
        case Token::LE:     return "LE";
        case Token::GE:     return "GE";
        case Token::AND:    return "AND";
        case Token::OR:     return "OR";
        case Token::NOT:    return "NOT";
        case Token::INC:    return "INC";
        case Token::DEC:    return "DEC";
        case Token::AMPERSAND:return "AMPERSAND";
        case Token::ARROW:  return "ARROW";
        case Token::DOT:    return "DOT";
        case Token::NUM:    return "NUM";
        case Token::FNUM:   return "FNUM";
        case Token::CHAR_LIT:return "CHAR_LIT";
        case Token::STRING_LIT:return "STRING_LIT";
        case Token::TRUE:   return "TRUE";
        case Token::FALSE:  return "FALSE";
        case Token::ID:     return "ID";
        case Token::PRINTF: return "PRINTF";
        case Token::END:    return "END";
        case Token::ERR:    return "ERR";
        default:            return "UNKNOWN";
    }
}

const char* token_type_to_string(Token::Type t) {
    switch (t) {
        case Token::LPAREN:    return "'('";
        case Token::RPAREN:    return "')'";
        case Token::LBRACE:    return "'{'";
        case Token::RBRACE:    return "'}'";
        case Token::LBRACKET:  return "'['";
        case Token::RBRACKET:  return "']'";
        case Token::SEMICOL:   return "';'";
        case Token::COMA:      return "','";
        case Token::COLON:     return "':'";
        case Token::PLUS:      return "'+'";
        case Token::MINUS:     return "'-'";
        case Token::STAR:      return "'*'";
        case Token::DIV:       return "'/'";
        case Token::MOD:       return "'%'";
        case Token::POW:       return "'**'";
        case Token::ASSIGN:    return "'='";
        case Token::EQ:        return "'=='";
        case Token::NE:        return "'!='";
        case Token::LT:        return "'<'";
        case Token::GT:        return "'>'";
        case Token::LE:        return "'<='";
        case Token::GE:        return "'>='";
        case Token::AND:       return "'&&'";
        case Token::OR:        return "'||'";
        case Token::NOT:       return "'!'";
        case Token::INC:       return "'++'";
        case Token::DEC:       return "'--'";
        case Token::AMPERSAND: return "'&'";
        case Token::ARROW:     return "'->'";
        case Token::DOT:       return "'.'";
        default:               return token_name(t);
    }
}

ostream& operator<<(ostream& outs, const Token& tok) {
    outs << "TOKEN(" << token_name(tok.type) << ", \"" << tok.text << "\"";
    if (tok.line > 0) outs << ", " << tok.line << ":" << tok.col;
    outs << ")";
    return outs;
}

ostream& operator<<(ostream& outs, const Token* tok) {
    if (!tok) return outs << "TOKEN(NULL)";
    return outs << *tok;
}
