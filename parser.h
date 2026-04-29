#ifndef PARSER_H
#define PARSER_H
#define MAX_VARS 100
#include "lexer.h"

typedef enum { TYPE_INT, TYPE_FLOAT, TYPE_BOOL, TYPE_CHAR } DataType;

typedef struct {
    char name[50];
    DataType type;
    union {
        int i;
        float f;
        char c;
    } value;
} Symbol;

// Tipos de nós
typedef enum {
    NODE_INT,
    NODE_FLOAT,
    NODE_BINOP,
    NODE_VAR,
    NODE_ASSIGN,
    NODE_IF,
    NODE_BLOCK,
    NODE_WHILE,
    NODE_DECL,
    NODE_CAST
} NodeType;

// Estrutura da árvore
typedef struct AST {
    NodeType type;

    union {
        // Valores Literais
        int intValue;
        float floatValue;
        char varName[50]; 

        // Operações Binárias (+, -, *, /, >, <, etc)
        struct {
            TokenType op;
            struct AST* left;
            struct AST* right;
        } binop;

        // Atribuição (x = valor)
        struct {
            char varName[50];
            struct AST* value;
        } assign;

        // Blocos de comandos { ... }
        struct {
            struct AST* children[100]; 
            int count;
        } block;

        // Estruturas de Controle (IF e WHILE)
        struct {
            struct AST* condition;
            struct AST* thenBranch;
            struct AST* elseBranch; // No WHILE, isso ficará como NULL
        } control;

        struct {
            char varName[50];
            DataType type;
        } decl;

        struct {
            DataType type;
            struct AST* expr;
        } cast;
    }; 
    // A union termina aqui. O 'type' lá em cima dirá qual desses campos usar.
} AST;

// Função principal
AST* parseExpression();
void advance();
float evaluate(AST* node);
AST* parseStatement();
AST* parseBlock();
AST* parseProgram();
void printSymbolTable();
char* nodeTypeToString(NodeType t);
AST* parseLogical();

#endif