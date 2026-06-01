#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int tacPrint = 1;
int varCount = 0;
int currentScope = 0;
AST* parseLogical();
int findVar(char* name);

Symbol symbolTable[MAX_VARS];


Token currentToken;

int tempCount = 0;
DataType tempTypes[100];

char* newTemp() {
    char* temp = malloc(10);
    sprintf(temp, "t%d", varCount + tempCount);
    tempCount++;
    return temp;
}

char* getVarTemp(char* name) {
    int pos = findVar(name);

    if (pos == -1) {
        printf("Erro: variavel %s nao declarada\n", name);
        exit(1);
    }

    char* temp = malloc(10);
    sprintf(temp, "t%d", pos);
    return temp;
}

DataType getNodeType(AST* node) {

    if (node->type == NODE_INT)
        return TYPE_INT;

    if (node->type == NODE_FLOAT)
        return TYPE_FLOAT;

    if (node->type == NODE_VAR) {
        int pos = findVar(node->varName);

        if (pos == -1) {
            printf("Erro: variavel nao declarada\n");
            exit(1);
        }

        return symbolTable[pos].type;
    }

    if (node->type == NODE_CHAR)
        return TYPE_CHAR;

    if (node->type == NODE_BOOL)
        return TYPE_BOOL;

    if (node->type == NODE_CAST)
        return node->cast.type;

    // BINOP
    if (node->type == NODE_BINOP) {

        DataType left = getNodeType(node->binop.left);
        DataType right = getNodeType(node->binop.right);

        // se um dos lados for float -> resultado float
        if (left == TYPE_FLOAT || right == TYPE_FLOAT)
            return TYPE_FLOAT;

        return TYPE_INT;
    }

    return TYPE_INT;
}

DataType getTempType(int index) {
    if (index < varCount) {
        return symbolTable[index].type;
    }

    return tempTypes[index];
}

//gera codigo de 3 endereços
char* generateTAC(AST* node) {
    if (node == NULL) return NULL;

    if (node->type == NODE_INT) {
        char* temp = newTemp();
        int num = atoi(temp + 1);
        tempTypes[num] = TYPE_INT;

        if (tacPrint)
            printf("%s = %d;\n", temp, node->intValue);

        return temp;
    }

    if (node->type == NODE_FLOAT) {
        char* temp = newTemp();
        int num = atoi(temp + 1);
        tempTypes[num] = TYPE_FLOAT;

        if (tacPrint)
            printf("%s = %.1f;\n", temp, node->floatValue);

        return temp;
    }

    if (node->type == NODE_STRING) {
        char* temp = malloc(255);
        sprintf(temp, "\"%s\"", node->stringValue);
        return temp; 
    }

    if (node->type == NODE_PRINT) {
        char* expr = generateTAC(node->printStmt.expr);
        if (tacPrint) printf("print %s;\n", expr);
        return NULL;
    }

    if (node->type == NODE_READ) {
        char* varTemp = getVarTemp(node->readStmt.varName);
        if (tacPrint) printf("read %s;\n", varTemp);
        return NULL;
    }

    if (node->type == NODE_CHAR) {
        char* temp = newTemp();
        int num = atoi(temp + 1);
        tempTypes[num] = TYPE_CHAR;

        if (tacPrint)
            printf("%s = '%c';\n", temp, node->charValue);

        return temp;
    }

    if (node->type == NODE_BOOL) {
        char* temp = newTemp();
        int num = atoi(temp + 1);
        tempTypes[num] = TYPE_INT; // bool vira int no código intermediário

        if (tacPrint)
            printf("%s = %d;\n", temp, node->boolValue);

        return temp;
    }

    if (node->type == NODE_VAR) {
        return getVarTemp(node->varName);
    }

    if (node->type == NODE_UNOP) {

        char* expr = generateTAC(node->unop.expr);

        char* temp = newTemp();

        int num = atoi(temp + 1);

        tempTypes[num] = TYPE_INT;

        if (node->unop.op == T_NOT) {

            if (tacPrint)
                printf("%s = !%s;\n", temp, expr);
        }

        return temp;
    }

    if (node->type == NODE_BINOP) {
    DataType leftType = getNodeType(node->binop.left);
    DataType rightType = getNodeType(node->binop.right);

    char* left = generateTAC(node->binop.left);
    char* right = generateTAC(node->binop.right);

    // conversão implícita: int -> float
    if (leftType == TYPE_INT && rightType == TYPE_FLOAT) {
        char* castTemp = newTemp();
        int castNum = atoi(castTemp + 1);

        tempTypes[castNum] = TYPE_FLOAT;

        if (tacPrint)
            printf("%s = (float) %s;\n", castTemp, left);

        left = castTemp;
    }

    if (leftType == TYPE_FLOAT && rightType == TYPE_INT) {
        char* castTemp = newTemp();
        int castNum = atoi(castTemp + 1);

        tempTypes[castNum] = TYPE_FLOAT;

        if (tacPrint)
            printf("%s = (float) %s;\n", castTemp, right);

        right = castTemp;
    }

    char* temp = newTemp();
    int num = atoi(temp + 1);

    char op[5];

    switch (node->binop.op) {
        case T_PLUS:  strcpy(op, "+"); break;
        case T_MINUS: strcpy(op, "-"); break;
        case T_MUL:   strcpy(op, "*"); break;
        case T_DIV:   strcpy(op, "/"); break;
        case T_GT:    strcpy(op, ">"); break;
        case T_LT:    strcpy(op, "<"); break;
        case T_EQ:    strcpy(op, "=="); break;
        case T_NEQ:   strcpy(op, "!="); break;
        case T_AND:   strcpy(op, "&&"); break;
        case T_OR:    strcpy(op, "||"); break;
        default:      strcpy(op, "?"); break;
    }

    if (node->binop.op == T_GT || node->binop.op == T_LT ||
        node->binop.op == T_EQ || node->binop.op == T_NEQ ||
        node->binop.op == T_AND || node->binop.op == T_OR) {
        tempTypes[num] = TYPE_INT;
    } else {
        tempTypes[num] = getNodeType(node);
    }

    if (tacPrint)
        printf("%s = %s %s %s;\n", temp, left, op, right);

    return temp;
}

    if (node->type == NODE_ASSIGN) {
        char* varTemp = getVarTemp(node->assign.varName);
        AST* value = node->assign.value;

        int varNum = atoi(varTemp + 1);

        // atribuição simples: NÃO cria temporário extra
        if (value->type == NODE_INT) {
            tempTypes[varNum] = TYPE_INT;

            if (tacPrint)
                printf("%s = %d;\n", varTemp, value->intValue);

            return NULL;
        }

        if (value->type == NODE_FLOAT) {
            tempTypes[varNum] = TYPE_FLOAT;

            if (tacPrint)
                printf("%s = %.1f;\n", varTemp, value->floatValue);

            return NULL;
        }

        if (value->type == NODE_CHAR) {
            tempTypes[varNum] = TYPE_CHAR;

            if (tacPrint)
                printf("%s = '%c';\n", varTemp, value->charValue);

            return NULL;
        }

        if (value->type == NODE_BOOL) {
            tempTypes[varNum] = TYPE_INT;

            if (tacPrint)
                printf("%s = %d;\n", varTemp, value->boolValue);

            return NULL;
        }

        // expressão composta: gera temporários normalmente
        char* val = generateTAC(value);

        //char* tempAssign = newTemp();
        //int num = atoi(tempAssign + 1);
        //tempTypes[num] = TYPE_INT;

        if (tacPrint)
            printf("%s = %s;\n", varTemp, val);

        return NULL;
    }

    if (node->type == NODE_DECL) {
        return NULL;
    }

    if (node->type == NODE_CAST) {

        char* expr = generateTAC(node->cast.expr);

        char* temp = newTemp();
        int num = atoi(temp + 1);

        // define o tipo do temporário
        tempTypes[num] = node->cast.type;

        char typeName[10];

        switch (node->cast.type) {
            case TYPE_INT:
                strcpy(typeName, "int");
                break;

            case TYPE_FLOAT:
                strcpy(typeName, "float");
                break;

            case TYPE_BOOL:
                strcpy(typeName, "bool");
                break;

            case TYPE_CHAR:
                strcpy(typeName, "char");
                break;

            default:
                strcpy(typeName, "?");
                break;
        }

        if (tacPrint)
            printf("%s = (%s) %s;\n", temp, typeName, expr);

        return temp;
    }

    if (node->type == NODE_BLOCK) {
        for (int i = 0; i < node->block.count; i++) {
            generateTAC(node->block.children[i]);
        }

        return NULL;
    }

    return NULL;
}

int countTemps(AST* node) {
    if (node == NULL) return 0;

    if (node->type == NODE_INT ||
        node->type == NODE_FLOAT ||
        node->type == NODE_CHAR ||
        node->type == NODE_BOOL)
        return 1;

    if (node->type == NODE_CAST) {
        return 1 + countTemps(node->cast.expr);
    }

    if (node->type == NODE_BINOP) {
        int total = 1 + countTemps(node->binop.left) + countTemps(node->binop.right);

        DataType leftType = getNodeType(node->binop.left);
        DataType rightType = getNodeType(node->binop.right);

        if ((leftType == TYPE_INT && rightType == TYPE_FLOAT) ||
            (leftType == TYPE_FLOAT && rightType == TYPE_INT)) {
            total++;
        }

        return total;
    }

    if (node->type == NODE_UNOP)
        return 1 + countTemps(node->unop.expr);

    if (node->type == NODE_ASSIGN) {
        AST* value = node->assign.value;

        if (value->type == NODE_INT ||
            value->type == NODE_FLOAT ||
            value->type == NODE_CHAR ||
            value->type == NODE_BOOL) {
            return 0;
        }

        return countTemps(value);
    }

    if (node->type == NODE_BLOCK) {
        int total = 0;
        for (int i = 0; i < node->block.count; i++)
            total += countTemps(node->block.children[i]);
        return total;
    }
    

    return 0;
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
    // Busca de trás para frente (prioriza o escopo mais interno)
    for (int i = varCount - 1; i >= 0; i--) {
        if (strcmp(symbolTable[i].name, name) == 0) {
            return i;
        }
    }
    return -1; // Não encontrou em nenhum escopo
}

void addSymbol(char* name, DataType type) {
    // Verifica se a variável já existe NO MESMO ESCOPO
    for (int i = varCount - 1; i >= 0; i--) {
        if (symbolTable[i].scope < currentScope) break; // Saiu do escopo atual, pode parar de procurar
        
        if (strcmp(symbolTable[i].name, name) == 0) {
            printf("Erro: Variavel '%s' ja declarada neste escopo!\n", name);
            exit(1);
        }
    }

    if (varCount < MAX_VARS) {
        strcpy(symbolTable[varCount].name, name);
        symbolTable[varCount].type = type;
        symbolTable[varCount].scope = currentScope; // Salva o escopo atual
        
        // Inicializa com zero
        if (type == TYPE_INT) symbolTable[varCount].value.i = 0;
        else if (type == TYPE_FLOAT) symbolTable[varCount].value.f = 0.0;
        else if (type == TYPE_CHAR) symbolTable[varCount].value.c = '\0';
        
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

AST* createUnOpNode(TokenType op, AST* expr) {
    AST* node = malloc(sizeof(AST));

    node->type = NODE_UNOP;
    node->unop.op = op;
    node->unop.expr = expr;

    return node;
}

//PARSER
AST* parseFactor() {

    if (currentToken.type == T_NOT) {
        advance();

        AST* expr = parseFactor();

        return createUnOpNode(T_NOT, expr);
    }

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

    if (currentToken.type == T_STRING) {
        node = createStringNode(currentToken.lexeme);
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

    if (currentToken.type == T_CHAR) {
        node = createCharNode(currentToken.lexeme[0]);
        advance();
        return node;
    }

    if (currentToken.type == T_BOOL) {

        int val =
            strcmp(currentToken.lexeme, "true") == 0;

        node = createBoolNode(val);

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

AST* createForNode(AST* init, AST* cond, AST* inc, AST* body) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_FOR;
    node->forLoop.init = init;
    node->forLoop.condition = cond;
    node->forLoop.increment = inc;
    node->forLoop.body = body;
    return node;
}

AST* createDoWhileNode(AST* body, AST* cond) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_DO_WHILE;
    node->doWhileLoop.body = body;
    node->doWhileLoop.condition = cond;
    return node;
}

AST* createBreakNode() {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_BREAK;
    return node;
}

AST* createContinueNode() {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_CONTINUE;
    return node;
}

AST* parseStatement() {

    // PARSING: BREAK e CONTINUE
    if (currentToken.type == T_BREAK) {
        advance();
        if (currentToken.type != T_SEMICOLON) { printf("Erro: esperado ';'\n"); exit(1); }
        advance();
        return createBreakNode();
    }

    if (currentToken.type == T_CONTINUE) {
        advance();
        if (currentToken.type != T_SEMICOLON) { printf("Erro: esperado ';'\n"); exit(1); }
        advance();
        return createContinueNode();
    }

    // PARSING: DO / WHILE
    if (currentToken.type == T_DO) {
        advance(); // pula "do"
        if (currentToken.type != T_LBRACE) { printf("Erro: esperado '{'\n"); exit(1); }
        advance();
        
        AST* body = parseBlock(); // lê o bloco de código
        
        if (currentToken.type != T_WHILE) { printf("Erro: esperado 'while'\n"); exit(1); }
        advance();
        if (currentToken.type != T_LPAREN) { printf("Erro: esperado '('\n"); exit(1); }
        advance();
        
        AST* cond = parseRelational();
        
        if (currentToken.type != T_RPAREN) { printf("Erro: esperado ')'\n"); exit(1); }
        advance();
        if (currentToken.type != T_SEMICOLON) { printf("Erro: esperado ';'\n"); exit(1); }
        advance();
        
        return createDoWhileNode(body, cond);
    }

    // PARSING: FOR
    if (currentToken.type == T_FOR) {
        advance(); // pula "for"
        if (currentToken.type != T_LPAREN) { printf("Erro: esperado '('\n"); exit(1); }
        advance();
        
        // Inicialização
        AST* init = parseStatement(); // parseStatement já consome o ';'
        
        // Condição
        AST* cond = parseRelational();
        if (currentToken.type != T_SEMICOLON) { printf("Erro: esperado ';'\n"); exit(1); }
        advance();
        
        // Incremento (Lemos como uma atribuição simples, mas sem o ';' no final)
        AST* inc = NULL;
        if (currentToken.type == T_ID) {
            char varName[100];
            strcpy(varName, currentToken.lexeme);
            advance();
            if (currentToken.type == T_ASSIGN) {
                advance();
                AST* val = parseLogical();
                inc = createAssignNode(varName, val);
            }
        }
        
        if (currentToken.type != T_RPAREN) { printf("Erro: esperado ')'\n"); exit(1); }
        advance();
        if (currentToken.type != T_LBRACE) { printf("Erro: esperado '{'\n"); exit(1); }
        advance();
        
        AST* body = parseBlock();
        
        return createForNode(init, cond, inc, body);
    }

    if (currentToken.type == T_PRINT) {
        advance(); // pula "print"
        if (currentToken.type != T_LPAREN) { printf("Erro: esperado '('\n"); exit(1); }
        advance();
        AST* expr = parseLogical();
        if (currentToken.type != T_RPAREN) { printf("Erro: esperado ')'\n"); exit(1); }
        advance();
        if (currentToken.type != T_SEMICOLON) { printf("Erro: esperado ';'\n"); exit(1); }
        advance();
        return createPrintNode(expr);
    }

    if (currentToken.type == T_READ) {
        advance(); // pula "read"
        if (currentToken.type != T_LPAREN) { printf("Erro: esperado '('\n"); exit(1); }
        advance();
        if (currentToken.type != T_ID) { printf("Erro: esperado identificador\n"); exit(1); }
        
        char varName[50];
        strcpy(varName, currentToken.lexeme);
        advance();
        
        if (currentToken.type != T_RPAREN) { printf("Erro: esperado ')'\n"); exit(1); }
        advance();
        if (currentToken.type != T_SEMICOLON) { printf("Erro: esperado ';'\n"); exit(1); }
        advance();
        return createReadNode(varName);
    }

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

        if (currentToken.type != T_SEMICOLON) {
            printf("Erro: esperado ';' apos declaracao de %s\n", name);
            exit(1);
        }

    advance();

    return createDeclNode(name, type);

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

    int variablesBeforeBlock = varCount; // Salva o estado da tabela antes do bloco
    currentScope++;                      // Entra em um novo escopo (Contexto Local)

    while (currentToken.type != T_RBRACE && currentToken.type != T_EOF) {
        if (currentToken.type == T_SEMICOLON) {
            advance();
            continue;
        }
        node->block.children[node->block.count++] = parseStatement();
    }

    if (currentToken.type == T_RBRACE) {
        advance();
    } else {
        printf("Erro: esperado '}'\n");
        exit(1);
    }

    currentScope--;                  // Volta para o escopo anterior
    varCount = variablesBeforeBlock; // Remove as variáveis locais da memória (Pilha)

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

        // NOVO CASO: expressão pura
        if (currentToken.type == T_INT ||
            currentToken.type == T_FLOAT ||
            currentToken.type == T_LPAREN) {

            AST* expr = parseLogical();

            // exige ;
            if (currentToken.type == T_SEMICOLON) {
                advance();
            }

            programNode->block.children[programNode->block.count++] = expr;
            continue;
        }
        programNode->block.children[programNode->block.count++] = parseStatement();

        
    }

    return programNode;
}

//cria nó char
AST* createCharNode(char value) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_CHAR;
    node->charValue = value;
    return node;
}

//cria nó boolean
AST* createBoolNode(int value) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_BOOL;
    node->boolValue = value;
    return node;
}

AST* createStringNode(char* value) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_STRING;
    strcpy(node->stringValue, value);
    return node;
}

AST* createPrintNode(AST* expr) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_PRINT;
    node->printStmt.expr = expr;
    return node;
}

AST* createReadNode(char* varName) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_READ;
    strcpy(node->readStmt.varName, varName);
    return node;
}

//funcao de avaliçao da arvore AST
float evaluate(AST* node) {

    if (node == NULL) return 0;
    //printf("DEBUG: Executando %s\n", nodeTypeToString(node->type));

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
        /*printf("LOG: INT %s = %d\n",
            symbolTable[pos].name,
            symbolTable[pos].value.i);*/

    } else if (symbolTable[pos].type == TYPE_FLOAT) {
        symbolTable[pos].value.f = (float) val;
        /*printf("LOG: FLOAT %s = %.2f\n",
            symbolTable[pos].name,
            symbolTable[pos].value.f);*/

    } else if (symbolTable[pos].type == TYPE_BOOL) {
        symbolTable[pos].value.i = (val != 0);
        /*printf("LOG: BOOL %s = %d\n",
            symbolTable[pos].name,
            symbolTable[pos].value.i);*/

    } else if (symbolTable[pos].type == TYPE_CHAR) {
        symbolTable[pos].value.c = (char)val;
        /*printf("LOG: CHAR %s = %c\n",
            symbolTable[pos].name,
            symbolTable[pos].value.c);*/
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

    //char
    if (node->type == NODE_CHAR) {
        return node->charValue;
    }

    //boolean
    if (node->type == NODE_BOOL) {
        return node->boolValue;
    }

    if (node->type == NODE_UNOP) {

        float val = evaluate(node->unop.expr);

        switch (node->unop.op) {
            case T_NOT:
                return !val;

            default:
                printf("Operador unario desconhecido\n");
                exit(1);
        }
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
// Imprime a indentação para manter o código C gerado organizado
void printIndent(int indent) {
    for(int i = 0; i < indent; i++) {
        printf("    ");
    }
}

// Gera o código C para Expressões (números, variáveis, operações, cast)
void generateC_expr(AST* node) {
    if (!node) return;

    switch(node->type) {
        case NODE_INT:
            printf("%d", node->intValue);
            break;
        case NODE_FLOAT:
            printf("%.1f", node->floatValue);
            break;
        case NODE_CHAR:
            printf("'%c'", node->charValue);
            break;
        case NODE_BOOL:
            printf("%s", node->boolValue ? "true" : "false");
            break;
        case NODE_VAR:
            printf("%s", node->varName);
            break;
        case NODE_BINOP:
            printf("(");
            generateC_expr(node->binop.left);
            switch(node->binop.op) {
                case T_PLUS:  printf(" + "); break;
                case T_MINUS: printf(" - "); break;
                case T_MUL:   printf(" * "); break;
                case T_DIV:   printf(" / "); break;
                case T_GT:    printf(" > "); break;
                case T_LT:    printf(" < "); break;
                case T_EQ:    printf(" == "); break;
                case T_NEQ:   printf(" != "); break;
                case T_AND:   printf(" && "); break;
                case T_OR:    printf(" || "); break;
                case NODE_STRING:   printf("\"%s\"", node->stringValue); break;
                default: break;
            }
            generateC_expr(node->binop.right);
            printf(")");
            break;
        case NODE_UNOP:
            if (node->unop.op == T_NOT) printf("!");
            generateC_expr(node->unop.expr);
            break;
        case NODE_CAST:
            printf("(");
            switch(node->cast.type) {
                case TYPE_INT:   printf("(int)"); break;
                case TYPE_FLOAT: printf("(float)"); break;
                case TYPE_CHAR:  printf("(char)"); break;
                case TYPE_BOOL:  printf("(bool)"); break;
            }
            generateC_expr(node->cast.expr);
            printf(")");
            break;
        default:
            break;
    }
}

// Gera o código C para Statements (Declarações, Blocos, If, While)
void generateC(AST* node, int indent) {
    if (!node) return;

    switch(node->type) {
        case NODE_BLOCK:
            for(int i = 0; i < node->block.count; i++) {
                generateC(node->block.children[i], indent);
            }
            break;

        case NODE_DECL:
            printIndent(indent);
            switch(node->decl.type) {
                case TYPE_INT:   printf("int "); break;
                case TYPE_FLOAT: printf("float "); break;
                case TYPE_CHAR:  printf("char "); break;
                case TYPE_BOOL:  printf("bool "); break;
            }
            printf("%s;\n", node->decl.varName);
            break;

        case NODE_PRINT:
            printIndent(indent);
            DataType t = getNodeType(node->printStmt.expr);
            if (t == TYPE_INT || t == TYPE_BOOL) printf("printf(\"%%d\\n\", ");
            else if (t == TYPE_FLOAT) printf("printf(\"%%.2f\\n\", ");
            else if (t == TYPE_CHAR) printf("printf(\"%%c\\n\", ");
            else if (t == TYPE_STRING) printf("printf(\"%%s\\n\", ");
            generateC_expr(node->printStmt.expr);
            printf(");\n");
            break;

        case NODE_READ:
            printIndent(indent);
            int pos = findVar(node->readStmt.varName);
            if (pos == -1) {
                printf("Erro: Variável não declarada na leitura.\n");
                exit(1);
            }
            DataType tr = symbolTable[pos].type;
            if (tr == TYPE_INT || tr == TYPE_BOOL) printf("scanf(\"%%d\", &%s);\n", node->readStmt.varName);
            else if (tr == TYPE_FLOAT) printf("scanf(\"%%f\", &%s);\n", node->readStmt.varName);
            else if (tr == TYPE_CHAR) printf("scanf(\" %%c\", &%s);\n", node->readStmt.varName);
            break;
        
        case NODE_ASSIGN:
            printIndent(indent);
            printf("%s = ", node->assign.varName);
            generateC_expr(node->assign.value);
            printf(";\n");
            break;

        case NODE_IF:
            printIndent(indent);
            printf("if (");
            generateC_expr(node->control.condition);
            printf(") {\n");
            
            if (node->control.thenBranch) {
                generateC(node->control.thenBranch, indent + 1);
            }
            
            printIndent(indent);
            printf("}\n");
            
            if (node->control.elseBranch) {
                printIndent(indent);
                printf("else {\n");
                generateC(node->control.elseBranch, indent + 1);
                printIndent(indent);
                printf("}\n");
            }
            break;

        case NODE_WHILE:
            printIndent(indent);
            printf("while (");
            generateC_expr(node->control.condition);
            printf(") {\n");
            
            if (node->control.thenBranch) {
                generateC(node->control.thenBranch, indent + 1);
            }
            
            printIndent(indent);
            printf("}\n");
            break;

        case NODE_BREAK:
            printIndent(indent);
            printf("break;\n");
            break;

        case NODE_CONTINUE:
            printIndent(indent);
            printf("continue;\n");
            break;

        case NODE_DO_WHILE:
            printIndent(indent);
            printf("do {\n");
            if (node->doWhileLoop.body) {
                generateC(node->doWhileLoop.body, indent + 1);
            }
            printIndent(indent);
            printf("} while (");
            generateC_expr(node->doWhileLoop.condition);
            printf(");\n");
            break;

        case NODE_FOR:
            printIndent(indent);
            printf("for (");
            // A inicialização gera um statement com ';' e quebra de linha. 
            // Para ficar na mesma linha do FOR no C, extraímos apenas a expressão se for atribuição.
            if (node->forLoop.init && node->forLoop.init->type == NODE_ASSIGN) {
                printf("%s = ", node->forLoop.init->assign.varName);
                generateC_expr(node->forLoop.init->assign.value);
            }
            printf("; ");
            
            generateC_expr(node->forLoop.condition);
            printf("; ");
            
            if (node->forLoop.increment && node->forLoop.increment->type == NODE_ASSIGN) {
                printf("%s = ", node->forLoop.increment->assign.varName);
                generateC_expr(node->forLoop.increment->assign.value);
            }
            printf(") {\n");
            
            if (node->forLoop.body) {
                generateC(node->forLoop.body, indent + 1);
            }
            printIndent(indent);
            printf("}\n");
            break;    

        // Se expressões soltas acabarem dentro do bloco (avulsas)
        case NODE_INT:
        case NODE_FLOAT:
        case NODE_CHAR:
        case NODE_BOOL:
        case NODE_VAR:
        case NODE_BINOP:
        case NODE_UNOP:
        case NODE_CAST:
            printIndent(indent);
            generateC_expr(node);
            printf(";\n");
            break;

        default:
            break;
    }
}

