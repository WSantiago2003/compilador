#ifndef LEXER_H
#define LEXER_H

typedef enum {

    // Literais
    T_ID,
    T_INT,
    T_FLOAT,
    T_STRING,
    T_CHAR,
    T_BOOL,

    // Símbolos
    T_SEMICOLON,
    T_COMMA,
    T_LPAREN,
    T_RPAREN,
    T_LBRACE,
    T_RBRACE,
    T_LBRACKET,
    T_RBRACKET,

    // Operadores aritméticos
    T_PLUS,
    T_MINUS,
    T_MUL,
    T_DIV,

    // Relacionais
    T_GT,
    T_LT,
    T_EQ,
    T_NEQ,

    // Lógicos
    T_AND,
    T_OR,
    T_NOT,

    // Atribuição
    T_ASSIGN,

    // Palavras-chave (controle)
    T_IF,
    T_ELSE,
    T_WHILE,
    T_FOR,
    T_DO,

    // Tipos
    T_INT_TYPE,
    T_FLOAT_TYPE,
    T_BOOL_TYPE,
    T_CHAR_TYPE,
    T_STRING_TYPE,

    T_EOF

} TokenType;

typedef struct {
    TokenType type;
    char lexeme[100];
    int line; // número da linha (pra erro)
} Token;

char* tokenToString(TokenType type);

Token nextToken();

extern char input[1000];

#endif