#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum TokenType {
    NONE,
    END_OF_FILE,
    NUMBER,
    IDENTIFIER,
    LPAREN,
    RPAREN,
    COLON,
    ASSIGN,
    PLUS,
    MINUS,
    TIMES,
    DIVIDE,
    COMMA,
    IF,
    THEN,
    END,
    WHILE,
    DO,
    VAR,
};

class Token {
public:
    std::string source;
    int line;
    int column;
    TokenType type;
    std::string text;
    int value;

    std::string tostring() const;
};

std::vector<Token> tokenize(const std::string &source);

#endif
