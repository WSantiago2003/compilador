#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// Tipos de nós
typedef enum {
    NODE_INT,
    NODE_FLOAT,
    NODE_BINOP,
    NODE_VAR,
    NODE_ASSIGN,
    NODE_IF
} NodeType;

// Estrutura da árvore
typedef struct AST {
    NodeType type;

    union {
        int intValue;
        float floatValue;

        char varName[50]; // variável

        struct {
            TokenType op;
            struct AST* left;
            struct AST* right;
        } binop;

        struct {
            char varName[50];
            struct AST* value;
        } assign;

    };
    struct {
        struct AST* condition;
        struct AST* thenBranch;
        struct AST* elseBranch;
    } ifNode;

} AST;

// Função principal
AST* parseExpression();
void advance();
float evaluate(AST* node);
AST* parseStatement();
void parseProgram();

#endif