#ifndef PARSER_H
#define PARSER_H
#define MAX_VARS 100
#include "lexer.h"


typedef enum { TYPE_INT, TYPE_FLOAT, TYPE_BOOL, TYPE_CHAR, TYPE_STRING } DataType;

typedef struct {
    char name[50];
    DataType type;
    int scope;
    int isActive;
    union {
        int i;
        float f;
        char c;
        char s[255];
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
    NODE_CAST,
    NODE_CHAR,
    NODE_BOOL,
    NODE_UNOP,
    NODE_STRING,
    NODE_PRINT,
    NODE_READ,
    NODE_FOR,
    NODE_DO_WHILE,
    NODE_SWITCH,
    NODE_CASE,
    NODE_BREAK,
    NODE_CONTINUE,

} NodeType;

// Estrutura da árvore
typedef struct AST {
    NodeType type;
    int symbolIndex;

    union {
        // Valores Literais
        int intValue;
        float floatValue;
        char varName[50]; 
        char charValue;
        int boolValue;
        

        // Operações Binárias (+, -, *, /, >, <, etc)
        struct {
            TokenType op;
            struct AST* left;
            struct AST* right;
        } binop;

        char stringValue[255]; // Para armazenar o texto da string

        struct {
            struct AST* expr;
        } printStmt;

        struct {
            char varName[50];
        } readStmt;

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

        struct {
            TokenType op;
            struct AST* expr;
        } unop;

        // For (init; cond; inc) { body }
        struct {
            struct AST* init;
            struct AST* condition;
            struct AST* increment;
            struct AST* body;
        } forLoop;

        // Do { body } while (cond);
        struct {
            struct AST* body;
            struct AST* condition;
        } doWhileLoop;

        // Switch (expr) { cases }
        struct {
            struct AST* expr;
            struct AST* cases[20]; // Suporta até 20 cases por switch
            int caseCount;
        } switchStmt;

        // case expr: body
        struct {
            struct AST* matchExpr; // Se for NULL, representa o "default"
            struct AST* body;
        } caseStmt;
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
int countTemps(AST* node);
char* generateTAC(AST* node);
AST* createCharNode(char value);
AST* createBoolNode(int value);
DataType getTempType(int index);


extern DataType tempTypes[100];
extern int varCount;
extern int labelCount;
extern int tempCount;
extern int tacPrint;
void printIndent(int indent);
void generateC_expr(AST* node);
void generateC(AST* node, int indent);
AST* createStringNode(char* value);
AST* createPrintNode(AST* expr);
AST* createReadNode(char* varName);
#endif