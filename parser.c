#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

AST* parseIf();
AST* parseBlock();

#define MAX_VARS 100

char varNames[MAX_VARS][50];
float varValues[MAX_VARS];
int varCount = 0;

Token currentToken;

//pegar próximo token
void advance() {
    currentToken = nextToken();
}

//criar nós
AST* createIntNode(int value) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_INT;
    node->intValue = value;
    return node;
}

AST* createFloatNode(float value) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_FLOAT;
    node->floatValue = value;
    return node;
}

AST* createBinOpNode(TokenType op, AST* left, AST* right) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_BINOP;
    node->binop.op = op;
    node->binop.left = left;
    node->binop.right = right;
    return node;
}

AST* createVarNode(char* name) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_VAR;
    strcpy(node->varName, name);
    return node;
}

AST* createAssignNode(char* name, AST* value) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_ASSIGN;
    strcpy(node->assign.varName, name);
    node->assign.value = value;
    return node;
}

int findVar(char* name) {
    for (int i = 0; i < varCount; i++) {
        if (strcmp(varNames[i], name) == 0)
            return i;
    }
    return -1;
}

AST* createIfNode(AST* cond, AST* thenB, AST* elseB) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_IF;
    node->ifNode.condition = cond;
    node->ifNode.thenBranch = thenB;
    node->ifNode.elseBranch = elseB;
    return node;
}

//PARSER
AST* parseFactor() {
    AST* node;

    if (currentToken.type == T_INT) {
        node = createIntNode(atoi(currentToken.lexeme));
        advance();
        return node;
    }

    if (currentToken.type == T_FLOAT) {
        node = createFloatNode(atof(currentToken.lexeme));
        advance();
        return node;
    }

    if (currentToken.type == T_LPAREN) {
        advance();
        node = parseExpression();

        if (currentToken.type != T_RPAREN) {
            printf("Erro: esperado ')'\n");
            exit(1);
        }

        advance();
        return node;
    }

    if (currentToken.type == T_ID) {
        AST* node = createVarNode(currentToken.lexeme);
        advance();
        return node;
    }

    printf("Erro: fator inválido\n");
    exit(1);

}

//termo(* e /)
AST* parseTerm() {
    AST* node = parseFactor();

    while (currentToken.type == T_MUL || currentToken.type == T_DIV) {
        TokenType op = currentToken.type;
        advance();
        AST* right = parseFactor();
        node = createBinOpNode(op, node, right);
    }

    return node;
}

//expressão(+ e -)
AST* parseExpression() {
    AST* node = parseTerm();

    while (currentToken.type == T_PLUS || currentToken.type == T_MINUS) {
        TokenType op = currentToken.type;
        advance();
        AST* right = parseTerm();
        node = createBinOpNode(op, node, right);
    }

    return node;
}

//relacionais
AST* parseRelational() {
    AST* node = parseExpression();

    while (currentToken.type == T_GT || currentToken.type == T_LT ||
           currentToken.type == T_EQ || currentToken.type == T_NEQ) {

        TokenType op = currentToken.type;
        advance();

        AST* right = parseExpression();
        node = createBinOpNode(op, node, right);
    }

    return node;
}

//parser do IF
AST* parseIf() {

    advance(); // pula "if"

    if (currentToken.type != T_LPAREN) {
        printf("Erro: esperado '('\n");
        exit(1);
    }

    advance();
    AST* condition = parseRelational();

    if (currentToken.type != T_RPAREN) {
        printf("Erro: esperado ')'\n");
        exit(1);
    }

    advance();

    // ===== THEN =====
    if (currentToken.type != T_LBRACE) {
        printf("Erro: esperado '{'\n");
        exit(1);
    }

    advance(); // entra no bloco
    AST* thenBranch = parseBlock(); // 👈 já consome o }

    // ===== ELSE (opcional) =====
    AST* elseBranch = NULL;

    if (currentToken.type == T_ELSE) {
        advance();

        if (currentToken.type != T_LBRACE) {
            printf("Erro: esperado '{' após else\n");
            exit(1);
        }

        advance(); // entra no bloco
        elseBranch = parseBlock(); // 👈 já consome o }
    }

    return createIfNode(condition, thenBranch, elseBranch);
}

AST* parseStatement() {

    // IF
    if (currentToken.type == T_IF) {
        return parseIf();
    }

    // ATRIBUIÇÃO
    if (currentToken.type == T_ID) {

        char varName[100];
        strcpy(varName, currentToken.lexeme);

        advance();

        if (currentToken.type == T_ASSIGN) {
            advance();

            AST* value = parseExpression();
            return createAssignNode(varName, value);
        } else {
            printf("Erro: esperado '='\n");
            exit(1);
        }
    }

    // 👇 IMPORTANTE: evita cair em expressão inválida
    printf("Erro: comando inválido\n");
    exit(1);
}

//parser de bloco
AST* parseBlock() {

    while (currentToken.type != T_RBRACE && currentToken.type != T_EOF) {

        AST* stmt = parseStatement();
        evaluate(stmt);

        if (currentToken.type == T_SEMICOLON) {
            advance();
        }
    }

    // 👇 CONSOME O '}'
    if (currentToken.type == T_RBRACE) {
        advance();
    } else {
        printf("Erro: esperado '}'\n");
        exit(1);
    }

    return createIntNode(0);
}



//multiplas instruçoes
void parseProgram() {

    advance();

    while (currentToken.type != T_EOF) {

        AST* stmt = parseStatement();

        if (stmt->type == NODE_IF) {
            evaluate(stmt);
        } else {
            float result = evaluate(stmt);
            printf("Resultado: %.2f\n", result);
        }

        if (currentToken.type == T_SEMICOLON) {
            advance();
        }
    }
}

//funcao de avaliçao da arvore AST
float evaluate(AST* node) {

    if (node == NULL) return 0;

    //if
    if (node->type == NODE_IF) {

        float cond = evaluate(node->ifNode.condition);

        if (cond != 0) {
            evaluate(node->ifNode.thenBranch);
        } else {
            if (node->ifNode.elseBranch != NULL) {
                evaluate(node->ifNode.elseBranch);
            }
        }

        return 0;
    }

    // número inteiro
    if (node->type == NODE_INT) {
        return node->intValue;
    }

    // número float
    if (node->type == NODE_FLOAT) {
        return node->floatValue;
    }

    // operação binária
    if (node->type == NODE_BINOP) {
        float left = evaluate(node->binop.left);
        float right = evaluate(node->binop.right);

        switch (node->binop.op) {
            case T_PLUS: return left + right;
            case T_MINUS: return left - right;
            case T_MUL: return left * right;
            case T_DIV: return left / right;
            case T_GT: return left > right;
            case T_LT: return left < right;
            case T_EQ: return left == right;
            case T_NEQ: return left != right;

            default:
                printf("Operador desconhecido\n");
                exit(1);
        }
    }

    // variável
    if (node->type == NODE_VAR) {
        int idx = findVar(node->varName);

        if (idx == -1) {
            printf("Erro: variável %s não definida\n", node->varName);
            exit(1);
        }

        return varValues[idx];
    }

    // atribuição
    if (node->type == NODE_ASSIGN) {
        float value = evaluate(node->assign.value);

        int idx = findVar(node->assign.varName);

        if (idx == -1) {
            strcpy(varNames[varCount], node->assign.varName);
            varValues[varCount] = value;
            varCount++;
        } else {
            varValues[idx] = value;
        }

        return value;
    }

    printf("Erro na avaliacao - tipo: %d\n", node->type);
    exit(1);

    
}