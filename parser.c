#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

AST* parseLogical();
int tempCount = 0;

//funcao criar temporários
char* newTemp() {
    char* temp = malloc(10);
    sprintf(temp, "t%d", tempCount++);
    return temp;
}

char* generateTAC(AST* node) {
    if (node == NULL) return NULL;

        //inteiros
        if (node->type == NODE_INT) {
        char* val = malloc(10);
        sprintf(val, "%d", node->intValue);
        return val;
    }
        //float
        if (node->type == NODE_FLOAT) {
        char* val = malloc(20);
        sprintf(val, "%.2f", node->floatValue);
        return val;
    }
        //variavel
        if (node->type == NODE_VAR) {
        return strdup(node->varName);
    }
        //binario
        if (node->type == NODE_BINOP) {
        char* left = generateTAC(node->binop.left);
        char* right = generateTAC(node->binop.right);

        char* temp = newTemp();

        char op[5];

        switch (node->binop.op) {
            case T_PLUS: strcpy(op, "+"); break;
            case T_MINUS: strcpy(op, "-"); break;
            case T_MUL: strcpy(op, "*"); break;
            case T_DIV: strcpy(op, "/"); break;
            case T_GT: strcpy(op, ">"); break;
            case T_LT: strcpy(op, "<"); break;
            case T_EQ: strcpy(op, "=="); break;
            case T_NEQ: strcpy(op, "!="); break;
            case T_AND: strcpy(op, "&&"); break;
            case T_OR: strcpy(op, "||"); break;
        }

        printf("%s = %s %s %s\n", temp, left, op, right);

        return temp;
    }
        //atribuiçao
        if (node->type == NODE_ASSIGN) {
        char* val = generateTAC(node->assign.value);
        printf("%s = %s\n", node->assign.varName, val);
        return NULL;
    }
        //bloco
        if (node->type == NODE_BLOCK) {
        for (int i = 0; i < node->block.count; i++) {
            generateTAC(node->block.children[i]);
        }
        return NULL;
    }

    return NULL;
}


char* nodeTypeToString(NodeType t) {
    switch(t) {
        case NODE_INT: return "INT";
        case NODE_FLOAT: return "FLOAT";
        case NODE_BINOP: return "BINOP";
        case NODE_VAR: return "VAR";
        case NODE_ASSIGN: return "ASSIGN";
        case NODE_IF: return "IF";
        case NODE_BLOCK: return "BLOCK";
        case NODE_WHILE: return "WHILE";
        case NODE_DECL: return "DECL";
        case NODE_CAST: return "CAST";
        default: return "UNKNOWN";
    }
}

Symbol symbolTable[MAX_VARS];
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
        if (strcmp(symbolTable[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void addSymbol(char* name, DataType type) {
    if (varCount < MAX_VARS) {
        strcpy(symbolTable[varCount].name, name);
        symbolTable[varCount].type = type;
        // Inicializa com zero dependendo do tipo
        if (type == TYPE_INT) symbolTable[varCount].value.i = 0;
        else symbolTable[varCount].value.f = 0.0;
        
        varCount++;
    } else {
        printf("Erro: Tabela de simbolos cheia!\n");
        exit(1);
    }
}

AST* createControl(AST* cond, AST* thenB, AST* elseB) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_IF;
    node->control.condition = cond;
    node->control.thenBranch = thenB;
    node->control.elseBranch = elseB;
    return node;
}

AST* createDeclNode(char* name, DataType type) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_DECL;
    strcpy(node->decl.varName, name);
    node->decl.type = type;
    return node;
}

AST* createCastNode(DataType type, AST* expr) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_CAST;
    node->cast.type = type;
    node->cast.expr = expr;
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

    //TENTA DETECTAR CAST
    if (currentToken.type == T_INT_TYPE ||
        currentToken.type == T_FLOAT_TYPE ||
        currentToken.type == T_BOOL_TYPE) {

        DataType type;

        if (currentToken.type == T_INT_TYPE) type = TYPE_INT;
        else if (currentToken.type == T_FLOAT_TYPE) type = TYPE_FLOAT;
        else type = TYPE_BOOL;

        advance();

        if (currentToken.type != T_RPAREN) {
            printf("Erro: esperado ')'\n");
            exit(1);
        }

        advance();

        AST* expr = parseFactor(); //aplica cast
        return createCastNode(type, expr);
    }

    //expressão normal
    AST* node = parseLogical();

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

//logicos
AST* parseLogical() {
    AST* node = parseRelational();

    while (currentToken.type == T_AND || currentToken.type == T_OR) {
        TokenType op = currentToken.type;
        advance();

        AST* right = parseRelational();
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

    //THEN
    if (currentToken.type != T_LBRACE) {
        printf("Erro: esperado '{'\n");
        exit(1);
    }

    advance(); // entra no bloco
    AST* thenBranch = parseBlock(); 

    //ELSE
    AST* elseBranch = NULL;

    if (currentToken.type == T_ELSE) {
        advance();

        if (currentToken.type != T_LBRACE) {
            printf("Erro: esperado '{' após else\n");
            exit(1);
        }

        advance(); // entra no bloco
        elseBranch = parseBlock();
    }

    return createControl(condition, thenBranch, elseBranch);
}

AST* parseStatement() {

    if (currentToken.type == T_INT_TYPE ||
        currentToken.type == T_FLOAT_TYPE ||
        currentToken.type == T_BOOL_TYPE ||
        currentToken.type == T_CHAR_TYPE) {

        DataType type;

        if (currentToken.type == T_INT_TYPE) type = TYPE_INT;
        else if (currentToken.type == T_FLOAT_TYPE) type = TYPE_FLOAT;
        else if (currentToken.type == T_BOOL_TYPE) type = TYPE_BOOL;
        else type = TYPE_CHAR;

        advance();

        if (currentToken.type != T_ID) {
            printf("Erro: esperado identificador\n");
            exit(1);
        }

        char name[50];
        strcpy(name, currentToken.lexeme);

        advance();

        return createDeclNode(name, type);
    }

    // 1. Pular pontos e vírgulas inúteis
    while (currentToken.type == T_SEMICOLON) {
        advance();
    }

    // 2. Se for IF, chama o parser do IF
    if (currentToken.type == T_IF) {
        return parseIf();
    }

    // 3. Se for um ID, significa que é uma ATRIBUIÇÃO
    if (currentToken.type == T_ID) {
    char varName[100];
    strcpy(varName, currentToken.lexeme);

    advance(); // pula ID

    if (currentToken.type == T_ASSIGN) {
        advance(); // pula '='

        AST* value = parseLogical();

        //; necessario
        if (currentToken.type != T_SEMICOLON) {
            printf("Erro: esperado ';' apos atribuicao de %s\n", varName);
            exit(1);
        }

        advance(); // consome ;

        return createAssignNode(varName, value);
    } else {
        printf("Erro: esperado '=' apos o identificador %s\n", varName);
        exit(1);
    }
}

    if (currentToken.type == T_WHILE) {
        advance();
        advance(); 
        AST* cond = parseRelational();
        advance();
        
        if (currentToken.type != T_LBRACE) { printf("Erro: esperado '{'\n"); exit(1); }
        advance();
        
        AST* body = parseBlock();
        
        AST* node = malloc(sizeof(AST));
        node->type = NODE_WHILE;
        node->control.condition = cond;
        node->control.thenBranch = body;
        node->control.elseBranch = NULL;
        return node;
    }

    // Se o token for um '=', um número ou qualquer outra coisa solta:
    printf("Erro: comando invalido. Nao se pode começar um comando com '%s'\n", currentToken.lexeme);
    printf("DEBUG: Tentando ler token tipo %d lexema '%s'\n", currentToken.type, currentToken.lexeme);
    exit(1);
}


//parser de bloco
AST* parseBlock() {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_BLOCK;
    node->block.count = 0;

    while (currentToken.type != T_RBRACE && currentToken.type != T_EOF) {
        // Ignorar pontos e vírgulas soltos
        if (currentToken.type == T_SEMICOLON) {
            advance();
            continue;
        }

        // Guarda o comando na lista do bloco
        node->block.children[node->block.count++] = parseStatement();
    }

    if (currentToken.type == T_RBRACE) {
        advance();
    } else {
        printf("Erro: esperado '}'\n");
        exit(1);
    }

    return node;
}

//multiplas instruçoes
AST* parseProgram() {
    advance(); // Pega o primeiro token

    AST* programNode = malloc(sizeof(AST));
    programNode->type = NODE_BLOCK;
    programNode->block.count = 0;

    while (currentToken.type != T_EOF) {
        
        if (currentToken.type == T_SEMICOLON) {
            advance();
            continue;
        }

        if (currentToken.lexeme[0] == '\0') {
            advance();
            continue;
        }

        programNode->block.children[programNode->block.count++] = parseStatement();
    }

    return programNode;
}

//funcao de avaliçao da arvore AST
float evaluate(AST* node) {

    if (node == NULL) return 0;
    printf("DEBUG: Executando %s\n", nodeTypeToString(node->type));

    if (node->type == NODE_DECL) {
        if (findVar(node->decl.varName) != -1) {
            printf("Erro: variável já declarada\n");
            exit(1);
        }

        addSymbol(node->decl.varName, node->decl.type);
        return 0;
    }

    //percorre o bloco
    if (node->type == 7 || node->type == NODE_BLOCK) {
        float res = 0;
        for (int i = 0; i < node->block.count; i++) {
            res = evaluate(node->block.children[i]);
        }
        return res;
    }

    //if
    if (node->type == 6 || node->type == NODE_IF) {
        if (evaluate(node->control.condition) != 0) {
            return evaluate(node->control.thenBranch);
        } else if (node->control.elseBranch != NULL) {
            return evaluate(node->control.elseBranch);
        }
        return 0;
    }

    // atribuição
    if (node->type == NODE_ASSIGN) {
    // 1. Avalia o lado direito
    float val = evaluate(node->assign.value);

    // 2. Procura variável 
    int pos = findVar(node->assign.varName);

    if (pos == -1) {
        printf("Erro: Variavel '%s' nao declarada!\n", node->assign.varName);
        exit(1);
    }

    // 3. Atribuição com conversão por tipo
    if (symbolTable[pos].type == TYPE_INT) {
        symbolTable[pos].value.i = (int)val;
        printf("LOG: INT %s = %d\n",
            symbolTable[pos].name,
            symbolTable[pos].value.i);

    } else if (symbolTable[pos].type == TYPE_FLOAT) {
        symbolTable[pos].value.f = (float) val;
        printf("LOG: FLOAT %s = %.2f\n",
            symbolTable[pos].name,
            symbolTable[pos].value.f);

    } else if (symbolTable[pos].type == TYPE_BOOL) {
        symbolTable[pos].value.i = (val != 0);
        printf("LOG: BOOL %s = %d\n",
            symbolTable[pos].name,
            symbolTable[pos].value.i);

    } else if (symbolTable[pos].type == TYPE_CHAR) {
        symbolTable[pos].value.c = (char)val;
        printf("LOG: CHAR %s = %c\n",
            symbolTable[pos].name,
            symbolTable[pos].value.c);
    }

    return val;
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
            case T_AND: return (left != 0 && right != 0);
            case T_OR:  return (left != 0 || right != 0);

            default:
                printf("Operador desconhecido\n");
                exit(1);
        }
    }

    //explicita
    if (node->type == NODE_CAST) {
        float val = evaluate(node->cast.expr);

        if (node->cast.type == TYPE_INT)
            return (int)val;
        if (node->cast.type == TYPE_FLOAT)
            return (float)val;
        if (node->cast.type == TYPE_BOOL)
            return (val != 0);
    }

    // variável
    if (node->type == NODE_VAR) {
        int pos = findVar(node->varName);
        if (pos != -1) {
            if (symbolTable[pos].type == TYPE_INT) {
                return (float)symbolTable[pos].value.i;
            } else {
                return symbolTable[pos].value.f;
            }
        }
        printf("Erro: Variavel %s nao encontrada!\n", node->varName);
        exit(1);
    }

    

    if (node->type == NODE_WHILE) {
        while (evaluate(node->control.condition) != 0) {
            evaluate(node->control.thenBranch);
        }
        return 0;
    }

    printf("Erro na avaliacao - tipo: %d\n", node->type);
    exit(1);

    
}