#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <setjmp.h> // NOVO: Para o Panic Mode!

int tacPrint = 1;
int varCount = 0;
int currentScope = 0;
int varNum = 0;
int labelCount = 0;

Token currentToken;

// --- VARIÁVEIS DO PANIC MODE ---
int errorCount = 0;
jmp_buf panicBuffer;

// --- SISTEMA DE RASTREAMENTO DE ERROS ---
void syntaxError(const char* format, ...) {
    printf("\n[!] ERRO DE SINTAXE (Linha %d, Coluna %d): ", currentToken.line, currentToken.column);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    
    errorCount++;
    longjmp(panicBuffer, 1); // Dispara o teletransporte do Panic Mode!
}

void semanticError(const char* format, ...) {
    printf("\n[!] ERRO SEMANTICO (Linha %d, Coluna %d): ", currentToken.line, currentToken.column);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    
    errorCount++;
    longjmp(panicBuffer, 1); // Dispara o teletransporte do Panic Mode!
}
// -------------------------------------------------------

char* newLabel() {
    char* label = malloc(10);
    sprintf(label, "L%d", labelCount++);
    return label;
}

void printTacAsComment(AST* node, int indent) {
    if (!node) return;
    int oldTacPrint = tacPrint;
    tacPrint = 0; 
    generateTAC(node);
    tacPrint = oldTacPrint;
}

char loopStartStack[50][10];
char loopEndStack[50][10];
int loopDepth = 0;

void pushLoop(char* start, char* end) {
    strcpy(loopStartStack[loopDepth], start);
    strcpy(loopEndStack[loopDepth], end);
    loopDepth++;
}

void popLoop() {
    if (loopDepth > 0) loopDepth--;
}

AST* parseLogical();
int findVar(char* name);

Symbol symbolTable[MAX_VARS];
int tempCount = 0;
DataType tempTypes[100];

char* newTemp() {
    char* temp = malloc(10);
    sprintf(temp, "t%d", varCount + tempCount);
    tempCount++;
    return temp;
}

char* getVarTempByIndex(int index) {
    char* temp = malloc(10);
    sprintf(temp, "t%d", index);
    return temp;
}

DataType getNodeType(AST* node) {
    if (node->type == NODE_VAR) return symbolTable[node->symbolIndex].type;
    if (node->type == NODE_STRING) return TYPE_STRING;
    if (node->type == NODE_INT) return TYPE_INT;
    if (node->type == NODE_FLOAT) return TYPE_FLOAT;
    if (node->type == NODE_ARRAY_ACCESS) return symbolTable[node->symbolIndex].type;
    if (node->type == NODE_CHAR) return TYPE_CHAR;
    if (node->type == NODE_BOOL) return TYPE_BOOL;
    if (node->type == NODE_CAST) return node->cast.type;

    if (node->type == NODE_BINOP) {
        DataType left = getNodeType(node->binop.left);
        DataType right = getNodeType(node->binop.right);
        if (left == TYPE_FLOAT || right == TYPE_FLOAT) return TYPE_FLOAT;
        return TYPE_INT;
    }
    return TYPE_INT;
}

DataType getTempType(int index) {
    if (index < varCount) return symbolTable[index].type;
    return tempTypes[index];
}

char* generateTAC(AST* node) {
    if (node == NULL) return NULL;

    if (node->type == NODE_IF) {
        char* cond = generateTAC(node->control.condition);
        char* lElse = newLabel();
        char* lEnd = newLabel();

        if (tacPrint == 1) printf("ifFalse %s goto %s;\n", cond, node->control.elseBranch ? lElse : lEnd);
        else if (tacPrint == 2) printf("    if (!%s) goto %s;\n", cond, node->control.elseBranch ? lElse : lEnd);
        
        generateTAC(node->control.thenBranch);
        
        if (node->control.elseBranch) {
            if (tacPrint == 1) {
                printf("goto %s;\n", lEnd);
                printf("%s:\n", lElse);
            } else if (tacPrint == 2) {
                printf("    goto %s;\n", lEnd); 
                printf("    %s:\n", lElse);    
            }
            generateTAC(node->control.elseBranch);
        } else {
            if (tacPrint == 1) printf("%s:\n", lElse);
            else if (tacPrint == 2) printf("    %s:\n", lElse);
        }
        
        if (tacPrint == 1) printf("%s:\n", lEnd);
        else if (tacPrint == 2) printf("    %s:\n", lEnd); 
        
        return NULL;
    }

    if (node->type == NODE_WHILE) {
        char* lStart = newLabel();
        char* lEnd = newLabel();
        pushLoop(lStart, lEnd); 

        if (tacPrint) printf("%s:\n", lStart);
        char* cond = generateTAC(node->control.condition);
        if (tacPrint) printf("ifFalse %s goto %s;\n", cond, lEnd);
        
        generateTAC(node->control.thenBranch);
        
        if (tacPrint) printf("goto %s;\n", lStart);
        if (tacPrint) printf("%s:\n", lEnd);
        
        popLoop();
        return NULL;
    }

    if (node->type == NODE_DO_WHILE) {
        char* lStart = newLabel();
        char* lCond = newLabel();
        char* lEnd = newLabel();
        pushLoop(lCond, lEnd); 

        if (tacPrint) printf("%s:\n", lStart);
        if (node->doWhileLoop.body) generateTAC(node->doWhileLoop.body);
        
        if (tacPrint) printf("%s:\n", lCond);
        char* cond = generateTAC(node->doWhileLoop.condition);
        if (tacPrint) printf("if %s goto %s;\n", cond, lStart); 
        
        if (tacPrint) printf("%s:\n", lEnd);
        
        popLoop();
        return NULL;
    }

    if (node->type == NODE_FOR) {
        char* lStart = newLabel();
        char* lInc = newLabel();
        char* lEnd = newLabel();
        
        if (node->forLoop.init) generateTAC(node->forLoop.init);
        
        if (tacPrint == 1) printf("%s:\n", lStart);
        else if (tacPrint == 2) printf("    %s:\n", lStart); 
        
        if (node->forLoop.condition) {
            char* cond = generateTAC(node->forLoop.condition);
            if (tacPrint == 1) printf("ifFalse %s goto %s;\n", cond, lEnd);
            else if (tacPrint == 2) printf("    if (!%s) goto %s;\n", cond, lEnd); 
        }

        pushLoop(lInc, lEnd);
        if (node->forLoop.body) generateTAC(node->forLoop.body);
        
        if (tacPrint == 1) printf("%s:\n", lInc);
        else if (tacPrint == 2) printf("    %s:\n", lInc); 
        
        if (node->forLoop.increment) generateTAC(node->forLoop.increment);
        
        if (tacPrint == 1) printf("goto %s;\n", lStart);
        else if (tacPrint == 2) printf("    goto %s;\n", lStart); 
        
        if (tacPrint == 1) printf("%s:\n", lEnd);
        else if (tacPrint == 2) printf("    %s:\n", lEnd); 
        
        popLoop();
        return NULL;
    }

    if (node->type == NODE_SWITCH) {
        char* val = generateTAC(node->switchStmt.expr);
        char* lEnd = newLabel();
        char* caseLabels[20];
        char* lDefault = lEnd;

        for(int i = 0; i < node->switchStmt.caseCount; i++) {
            caseLabels[i] = newLabel();
            AST* c = node->switchStmt.cases[i];
            if (c->caseStmt.matchExpr) {
                char* matchVal = generateTAC(c->caseStmt.matchExpr);
                char* tCmp = newTemp();
                if (tacPrint) {
                    printf("%s = %s == %s;\n", tCmp, val, matchVal);
                    printf("if %s goto %s;\n", tCmp, caseLabels[i]);
                }
            } else {
                lDefault = caseLabels[i];
            }
        }
        if (tacPrint) printf("goto %s;\n", lDefault);

        pushLoop(loopDepth > 0 ? loopStartStack[loopDepth - 1] : "L_DUMMY", lEnd); 
        
        for(int i = 0; i < node->switchStmt.caseCount; i++) {
            if (tacPrint) printf("%s:\n", caseLabels[i]);
            AST* c = node->switchStmt.cases[i];
            if (c->caseStmt.body) generateTAC(c->caseStmt.body);
        }
        
        popLoop();
        if (tacPrint) printf("%s:\n", lEnd);
        return NULL;
    }

    if (node->type == NODE_BREAK) {
        if (tacPrint && loopDepth > 0) printf("goto %s;\n", loopEndStack[loopDepth - 1]);
        return NULL;
    }

    if (node->type == NODE_CONTINUE) {
        if (tacPrint && loopDepth > 0) printf("goto %s;\n", loopStartStack[loopDepth - 1]);
        return NULL;
    }

    if (node->type == NODE_INT) {
        char* temp = newTemp();
        int num = atoi(temp + 1);
        tempTypes[num] = TYPE_INT;

        if (tacPrint) printf("%s = %d;\n", temp, node->intValue);
        return temp;
    }

    if (node->type == NODE_FLOAT) {
        char* temp = newTemp();
        int num = atoi(temp + 1);
        tempTypes[num] = TYPE_FLOAT;

        if (tacPrint) printf("%s = %.1f;\n", temp, node->floatValue);
        return temp;
    }

    if (node->type == NODE_STRING) {
        char* temp = malloc(255);
        sprintf(temp, "\"%s\"", node->stringValue);
        return temp; 
    }

    if (node->type == NODE_PRINT) {
        if (node->printStmt.expr->type == NODE_STRING) {
            char* expr = generateTAC(node->printStmt.expr);
            if (tacPrint == 1) printf("print %s;\n", expr);
            else if (tacPrint == 2) printf("    printf(\"%%s\\n\", %s);\n", expr);
            return NULL;
        }

        char* expr = generateTAC(node->printStmt.expr);
        
        if (tacPrint == 1) {
            printf("print %s;\n", expr);
        } else if (tacPrint == 2) {
            int num = atoi(expr + 1); 
            DataType type = getTempType(num);

            if (type == TYPE_CHAR) printf("    printf(\"%%c\\n\", %s);\n", expr);
            else if (type == TYPE_STRING) printf("    printf(\"%%s\\n\", %s);\n", expr); 
            else printf("    printf(\"%%d\\n\", %s);\n", expr);
        }
        return NULL;
    }

    if (node->type == NODE_READ) {
        char* varTemp = getVarTempByIndex(node->symbolIndex);
        
        if (tacPrint == 1) {
            printf("read %s;\n", varTemp);
        } else if (tacPrint == 2) {
            DataType t = symbolTable[node->symbolIndex].type;
            if (t == TYPE_STRING) {
                printf("    scanf(\" %%254[^\\n]\", %s);\n", varTemp); 
                printf("    %s_len = strlen(%s);\n", varTemp, varTemp);
            } else if (t == TYPE_CHAR) {
                printf("    scanf(\" %%c\", &%s);\n", varTemp);
            } else if (t == TYPE_FLOAT) {
                printf("    scanf(\"%%f\", &%s);\n", varTemp);
            } else {
                printf("    scanf(\"%%d\", &%s);\n", varTemp);
            }
        }
        return NULL;
    }

    if (node->type == NODE_CHAR) {
        char* temp = newTemp();
        int num = atoi(temp + 1);
        tempTypes[num] = TYPE_CHAR;

        if (tacPrint) printf("%s = '%c';\n", temp, node->charValue);
        return temp;
    }

    if (node->type == NODE_BOOL) {
        char* temp = newTemp();
        int num = atoi(temp + 1);
        tempTypes[num] = TYPE_INT; 

        if (tacPrint) printf("%s = %d;\n", temp, node->boolValue);
        return temp;
    }

    if (node->type == NODE_VAR) {
        return getVarTempByIndex(node->symbolIndex);
    }

    if (node->type == NODE_UNOP) {
        char* expr = generateTAC(node->unop.expr);
        char* temp = newTemp();
        if (tacPrint) {
            if (node->unop.op == T_NOT) printf("%s = !%s;\n", temp, expr);
        }
        return temp;
    }

    if (node->type == NODE_BINOP) {
        DataType leftType = getNodeType(node->binop.left);
        DataType rightType = getNodeType(node->binop.right);

        char* left = generateTAC(node->binop.left);
        char* right = generateTAC(node->binop.right);

        if (leftType == TYPE_INT && rightType == TYPE_FLOAT) {
            char* castTemp = newTemp();
            int castNum = atoi(castTemp + 1);
            tempTypes[castNum] = TYPE_FLOAT;
            if (tacPrint == 1) printf("%s = (float) %s;\n", castTemp, left);
            else if (tacPrint == 2) printf("    %s = (float) %s;\n", castTemp, left);
            left = castTemp;
        }

        if (leftType == TYPE_FLOAT && rightType == TYPE_INT) {
            char* castTemp = newTemp();
            int castNum = atoi(castTemp + 1);
            tempTypes[castNum] = TYPE_FLOAT;
            if (tacPrint == 1) printf("%s = (float) %s;\n", castTemp, right);
            else if (tacPrint == 2) printf("    %s = (float) %s;\n", castTemp, right);
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
            case T_GTE:   strcpy(op, ">="); break; 
            case T_LTE:   strcpy(op, "<="); break; 
            case T_EQ:    strcpy(op, "=="); break;
            case T_NEQ:   strcpy(op, "!="); break;
            case T_AND:   strcpy(op, "&&"); break;
            case T_OR:    strcpy(op, "||"); break;
            default:      strcpy(op, "?"); break;
        }

        if (node->binop.op == T_GT || node->binop.op == T_LT ||
            node->binop.op == T_GTE || node->binop.op == T_LTE ||
            node->binop.op == T_EQ || node->binop.op == T_NEQ ||
            node->binop.op == T_AND || node->binop.op == T_OR) {
            tempTypes[num] = TYPE_INT;
        } else {
            tempTypes[num] = getNodeType(node);
        }

        if (tacPrint == 1) printf("%s = %s %s %s;\n", temp, left, op, right);
        else if (tacPrint == 2) printf("    %s = %s %s %s;\n", temp, left, op, right);

        return temp;
    }

    if (node->type == NODE_ARRAY_ACCESS) {
        char* idx = generateTAC(node->arrayAccess.index);
        char* idx2 = node->arrayAccess.index2 ? generateTAC(node->arrayAccess.index2) : NULL;
        char* varTemp = getVarTempByIndex(node->symbolIndex);
        char* temp = newTemp();
        int num = atoi(temp + 1);
        tempTypes[num] = symbolTable[node->symbolIndex].type;

        if (tacPrint == 1) {
            if (idx2) printf("%s = %s[%s][%s];\n", temp, varTemp, idx, idx2);
            else printf("%s = %s[%s];\n", temp, varTemp, idx);
        } else if (tacPrint == 2) {
            if (idx2) printf("    %s = %s[%s][%s];\n", temp, varTemp, idx, idx2);
            else printf("    %s = %s[%s];\n", temp, varTemp, idx);
        }
        return temp;
    }

    if (node->type == NODE_ARRAY_ASSIGN) {
        char* idx = generateTAC(node->arrayAssign.index);
        char* idx2 = node->arrayAssign.index2 ? generateTAC(node->arrayAssign.index2) : NULL;
        char* val = generateTAC(node->arrayAssign.value);
        char* varTemp = getVarTempByIndex(node->symbolIndex);
        
        if (tacPrint == 1) {
            if (idx2) printf("%s[%s][%s] = %s;\n", varTemp, idx, idx2, val);
            else printf("%s[%s] = %s;\n", varTemp, idx, val);
        } else if (tacPrint == 2) {
            if (idx2) printf("    %s[%s][%s] = %s;\n", varTemp, idx, idx2, val);
            else printf("    %s[%s] = %s;\n", varTemp, idx, val);
        }
        return NULL;
    }

   if (node->type == NODE_ASSIGN) {
        char* varTemp = getVarTempByIndex(node->symbolIndex);
        AST* value = node->assign.value;

        if (symbolTable[node->symbolIndex].type == TYPE_STRING) {
            char* val = generateTAC(value);
            int strLen = getStringSize(node->symbolIndex) - 1; 

            if (tacPrint == 1) {
                printf("%s = %s;\n", varTemp, val);
                printf("%s_len = %d;\n", varTemp, strLen); 
            } else if (tacPrint == 2) {
                printf("    strcpy(%s, %s);\n", varTemp, val);
                printf("    %s_len = %d;\n", varTemp, strLen); 
            }
            return NULL;
        }
        
        if (value->type == NODE_INT) {
            tempTypes[varNum] = TYPE_INT;
            if (tacPrint == 1) printf("%s = %d;\n", varTemp, value->intValue);
            else if (tacPrint == 2) printf("    %s = %d;\n", varTemp, value->intValue); 
            return NULL;
        }

        if (value->type == NODE_FLOAT) {
            tempTypes[varNum] = TYPE_FLOAT;
            if (tacPrint == 1) printf("%s = %.1f;\n", varTemp, value->floatValue);
            else if (tacPrint == 2) printf("    %s = %.1f;\n", varTemp, value->floatValue); 
            return NULL;
        }

        if (value->type == NODE_CHAR) {
            tempTypes[varNum] = TYPE_CHAR;
            if (tacPrint == 1) printf("%s = '%c';\n", varTemp, value->charValue);
            else if (tacPrint == 2) printf("    %s = '%c';\n", varTemp, value->charValue); 
            return NULL;
        }

        if (value->type == NODE_BOOL) {
            tempTypes[varNum] = TYPE_INT;
            if (tacPrint == 1) printf("%s = %d;\n", varTemp, value->boolValue);
            else if (tacPrint == 2) printf("    %s = %d;\n", varTemp, value->boolValue); 
            return NULL;
        }

        char* val = generateTAC(value);

        if (tacPrint == 1) printf("%s = %s;\n", varTemp, val);
        else if (tacPrint == 2) printf("    %s = %s;\n", varTemp, val); 

        return NULL;
    }

    if (node->type == NODE_DECL) return 0;

    if (node->type == NODE_CAST) {
        char* expr = generateTAC(node->cast.expr);
        char* temp = newTemp();
        int num = atoi(temp + 1);

        tempTypes[num] = node->cast.type;
        char typeName[10];

        switch (node->cast.type) {
            case TYPE_INT: strcpy(typeName, "int"); break;
            case TYPE_FLOAT: strcpy(typeName, "float"); break;
            case TYPE_BOOL: strcpy(typeName, "bool"); break;
            case TYPE_CHAR: strcpy(typeName, "char"); break;
            default: strcpy(typeName, "?"); break;
        }

        if (tacPrint) printf("%s = (%s) %s;\n", temp, typeName, expr);
        return temp;
    }

    if (node->type == NODE_BLOCK) {
        for (int i = 0; i < node->block.count; i++) generateTAC(node->block.children[i]);
        return NULL;
    }

    return NULL;
}

int countTemps(AST* node) {
    if (node == NULL) return 0;

    if (node->type == NODE_INT || node->type == NODE_FLOAT ||
        node->type == NODE_CHAR || node->type == NODE_BOOL)
        return 1;

    if (node->type == NODE_CAST) return 1 + countTemps(node->cast.expr);

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

    if (node->type == NODE_UNOP) return countTemps(node->unop.expr) + 1;
    if (node->type == NODE_PRINT) return countTemps(node->printStmt.expr);

    if (node->type == NODE_ASSIGN) {
        AST* value = node->assign.value;
        if (value->type == NODE_INT || value->type == NODE_FLOAT ||
            value->type == NODE_CHAR || value->type == NODE_BOOL) return 0;
        return countTemps(value);
    }

    if (node->type == NODE_IF) {
        int total = countTemps(node->control.condition) + countTemps(node->control.thenBranch);
        if (node->control.elseBranch) total += countTemps(node->control.elseBranch);
        return total;
    }

    if (node->type == NODE_WHILE) return countTemps(node->control.condition) + countTemps(node->control.thenBranch);

    if (node->type == NODE_DO_WHILE) {
        int total = 0;
        if (node->doWhileLoop.body) total += countTemps(node->doWhileLoop.body);
        if (node->doWhileLoop.condition) total += countTemps(node->doWhileLoop.condition);
        return total;
    }

    if (node->type == NODE_FOR) {
        int total = 0;
        if (node->forLoop.init) total += countTemps(node->forLoop.init);
        if (node->forLoop.condition) total += countTemps(node->forLoop.condition);
        if (node->forLoop.increment) total += countTemps(node->forLoop.increment);
        if (node->forLoop.body) total += countTemps(node->forLoop.body);
        return total;
    }

    if (node->type == NODE_SWITCH) {
        int total = 0;
        if (node->switchStmt.expr) total += countTemps(node->switchStmt.expr);
        for(int i = 0; i < node->switchStmt.caseCount; i++) {
            if(node->switchStmt.cases[i]->caseStmt.matchExpr) {
                total += countTemps(node->switchStmt.cases[i]->caseStmt.matchExpr) + 1; 
            }
            if(node->switchStmt.cases[i]->caseStmt.body) {
                total += countTemps(node->switchStmt.cases[i]->caseStmt.body);
            }
        }
        return total;
    }

    if (node->type == NODE_BLOCK) {
        int total = 0;
        for (int i = 0; i < node->block.count; i++) total += countTemps(node->block.children[i]);
        return total;
    }
    
    if (node->type == NODE_ARRAY_ACCESS) {
        int c = 1 + countTemps(node->arrayAccess.index);
        if (node->arrayAccess.index2) c += countTemps(node->arrayAccess.index2);
        return c;
    }
    if (node->type == NODE_ARRAY_ASSIGN) {
        int c = countTemps(node->arrayAssign.index) + countTemps(node->arrayAssign.value);
        if (node->arrayAssign.index2) c += countTemps(node->arrayAssign.index2);
        return c;
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

void advance() {
    currentToken = nextToken();
}

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
    
    node->symbolIndex = findVar(name);
    if (node->symbolIndex == -1) {
        semanticError("Variavel '%s' nao declarada!", name);
    }
    if (symbolTable[node->symbolIndex].isInitialized == 0) {
        printf("\n[!] AVISO (Linha %d, Coluna %d): Variavel '%s' lida sem ser inicializada.\n", 
                currentToken.line, currentToken.column, name);
    }
    symbolTable[node->symbolIndex].isUsed = 1;
    return node;
}

AST* createAssignNode(char* name, AST* value) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_ASSIGN;
    strcpy(node->assign.varName, name);
    node->assign.value = value;
    
    node->symbolIndex = findVar(name);
    
    if (node->symbolIndex == -1) {
        semanticError("Variavel '%s' nao declarada na atribuicao!", name);
    }
    
    symbolTable[node->symbolIndex].isInitialized = 1;
    
    return node;
}

int findVar(char* name) {
    for (int i = varCount - 1; i >= 0; i--) {
        if (symbolTable[i].isActive && strcmp(symbolTable[i].name, name) == 0) {
            return i;
        }
    }
    return -1; 
}

int addSymbol(char* name, DataType type, int isArray, int arraySize, int arrayCols) {
    for (int i = varCount - 1; i >= 0; i--) {
        if (!symbolTable[i].isActive) continue; 
        if (symbolTable[i].scope < currentScope) break; 
        
        if (strcmp(symbolTable[i].name, name) == 0) {
            semanticError("Variavel '%s' ja declarada neste escopo!", name);
        }
    }

    if (varCount < MAX_VARS) {
        strcpy(symbolTable[varCount].name, name);
        symbolTable[varCount].type = type;
        symbolTable[varCount].scope = currentScope;
        symbolTable[varCount].isActive = 1; 
        symbolTable[varCount].isArray = isArray;      
        symbolTable[varCount].arraySize = arraySize;  
        symbolTable[varCount].arrayCols = arrayCols;
        symbolTable[varCount].line = currentToken.line;
        symbolTable[varCount].isInitialized = 0;
        symbolTable[varCount].isUsed = 0;
        
        if (type == TYPE_INT) symbolTable[varCount].value.i = 0;
        else if (type == TYPE_FLOAT) symbolTable[varCount].value.f = 0.0;
        else if (type == TYPE_CHAR) symbolTable[varCount].value.c = '\0';
        else if (type == TYPE_STRING) strcpy(symbolTable[varCount].value.s, "");
        
        varCount++;
        return varCount - 1; 
    } else {
        semanticError("Tabela de simbolos cheia!");
    }
    return -1;
}

AST* createControl(AST* cond, AST* thenB, AST* elseB) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_IF;
    node->control.condition = cond;
    node->control.thenBranch = thenB;
    node->control.elseBranch = elseB;
    return node;
}

AST* createDeclNode(char* name, DataType type, int isArray, int arraySize, int arrayCols) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_DECL;
    strcpy(node->decl.varName, name);
    node->decl.type = type;
    node->decl.isArray = isArray;
    node->decl.arraySize = arraySize;
    node->decl.arrayCols = arrayCols;
    
    node->symbolIndex = addSymbol(name, type, isArray, arraySize, arrayCols);
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

AST* parseFactor() {
    if (currentToken.type == T_NOT) {
        advance();
        AST* expr = parseFactor();
        AST* node = malloc(sizeof(AST));
        node->type = NODE_UNOP;
        node->unop.op = T_NOT;
        node->unop.expr = expr;
        return node;
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
        if (currentToken.type == T_INT_TYPE || currentToken.type == T_FLOAT_TYPE || currentToken.type == T_BOOL_TYPE) {
            DataType type;
            if (currentToken.type == T_INT_TYPE) type = TYPE_INT;
            else if (currentToken.type == T_FLOAT_TYPE) type = TYPE_FLOAT;
            else type = TYPE_BOOL;

            advance();
            if (currentToken.type != T_RPAREN) syntaxError("esperado ')' apos cast");
            advance();
            AST* expr = parseFactor(); 
            return createCastNode(type, expr);
        }

        AST* node = parseLogical();
        if (currentToken.type != T_RPAREN) syntaxError("esperado ')'");
        advance();
        return node;
    }

    if (currentToken.type == T_CHAR) {
        node = createCharNode(currentToken.lexeme[0]);
        advance();
        return node;
    }

    if (currentToken.type == T_BOOL) {
        int val = strcmp(currentToken.lexeme, "true") == 0;
        node = createBoolNode(val);
        advance();
        return node;
    }

    if (currentToken.type == T_ID) {
        char varName[100];
        strcpy(varName, currentToken.lexeme);
        advance();
        
        if (currentToken.type == T_LBRACKET) {
            advance(); 
            AST* indexExpr = parseLogical(); 
            if (currentToken.type != T_RBRACKET) syntaxError("esperado ']'");
            advance(); 
            
            AST* index2Expr = NULL;
            if (currentToken.type == T_LBRACKET) {
                advance();
                index2Expr = parseLogical();
                if (currentToken.type != T_RBRACKET) syntaxError("esperado ']'");
                advance();
            }
            return createArrayAccessNode(varName, indexExpr, index2Expr);
        }
        return createVarNode(varName);
    }

    syntaxError("fator invalido (token inesperado '%s')", currentToken.lexeme);
    return NULL;
}

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

AST* parseRelational() {
    AST* node = parseExpression();
    while (currentToken.type == T_GT || currentToken.type == T_LT ||
            currentToken.type == T_GTE || currentToken.type == T_LTE ||
            currentToken.type == T_EQ || currentToken.type == T_NEQ) {
        TokenType op = currentToken.type;
        advance();
        AST* right = parseExpression();
        node = createBinOpNode(op, node, right);
    }
    return node;
}

AST* parseLogical() {
    AST* node = parseRelational(); 
    while (currentToken.type == T_AND || currentToken.type == T_OR) {
        TokenType op = currentToken.type;
        advance();
        AST* right = parseRelational();
        
        AST* parent = malloc(sizeof(AST));
        parent->type = NODE_BINOP;
        parent->binop.op = op;
        parent->binop.left = node;
        parent->binop.right = right;
        node = parent;
    }
    return node;
}

AST* parseIf() {
    advance(); 
    if (currentToken.type != T_LPAREN) syntaxError("esperado '(' apos 'if'");
    advance();
    AST* condition = parseLogical();
    if (currentToken.type != T_RPAREN) syntaxError("esperado ')' apos a condicao do if");
    advance();

    if (currentToken.type != T_LBRACE) syntaxError("esperado '{' para iniciar o bloco if");
    advance(); 
    AST* thenBranch = parseBlock(); 

    AST* elseBranch = NULL;
    if (currentToken.type == T_ELSE) {
        advance();
        if (currentToken.type == T_IF) {
            elseBranch = parseIf(); 
        } else {
            if (currentToken.type != T_LBRACE) syntaxError("esperado '{' apos 'else'");
            advance(); 
            elseBranch = parseBlock();
        }
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

AST* createSwitchNode(AST* expr) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_SWITCH;
    node->switchStmt.expr = expr;
    node->switchStmt.caseCount = 0;
    return node;
}

AST* createCaseNode(AST* matchExpr, AST* body) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_CASE;
    node->caseStmt.matchExpr = matchExpr;
    node->caseStmt.body = body;
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

AST* createArrayAccessNode(char* name, AST* indexExpr, AST* index2Expr) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_ARRAY_ACCESS;
    node->arrayAccess.index = indexExpr;
    node->arrayAccess.index2 = index2Expr;
    
    node->symbolIndex = findVar(name);
    if (node->symbolIndex == -1) semanticError("Vetor/Matriz '%s' nao declarado!", name);

    if (symbolTable[node->symbolIndex].isInitialized == 0) {
        printf("\n[!] AVISO (Linha %d, Coluna %d): Vetor/Matriz '%s' lido sem ser inicializado.\n", 
                currentToken.line, currentToken.column, name);
    }
    symbolTable[node->symbolIndex].isUsed = 1;
    
    int isArr = symbolTable[node->symbolIndex].isArray;
    
    if (isArr == 1 && index2Expr != NULL) semanticError("'%s' e um vetor 1D, mas recebeu 2 indices!", name);
    if (isArr == 2 && index2Expr == NULL) semanticError("'%s' e uma matriz 2D, precisa de 2 indices!", name);

    if (indexExpr->type == NODE_INT) {
        int idx = indexExpr->intValue;
        int max = symbolTable[node->symbolIndex].arraySize;
        if (idx < 0 || idx >= max) semanticError("Indice/Linha %d fora dos limites de '%s' (Max: %d)!", idx, name, max);
    }
    
    if (isArr == 2 && index2Expr->type == NODE_INT) {
        int idx2 = index2Expr->intValue;
        int max2 = symbolTable[node->symbolIndex].arrayCols;
        if (idx2 < 0 || idx2 >= max2) semanticError("Coluna %d fora dos limites de '%s' (Max: %d)!", idx2, name, max2);
    }
    return node;
}

AST* createArrayAssignNode(char* name, AST* indexExpr, AST* index2Expr, AST* value) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_ARRAY_ASSIGN;
    strcpy(node->arrayAssign.varName, name);
    node->arrayAssign.index = indexExpr;
    node->arrayAssign.index2 = index2Expr;
    node->arrayAssign.value = value;
    
    node->symbolIndex = findVar(name);
    if (node->symbolIndex == -1) semanticError("Vetor/Matriz '%s' nao declarado!", name);
    symbolTable[node->symbolIndex].isInitialized = 1;

    int isArr = symbolTable[node->symbolIndex].isArray;
    
    if (isArr == 1 && index2Expr != NULL) semanticError("'%s' e um vetor 1D, mas recebeu 2 indices!", name);
    if (isArr == 2 && index2Expr == NULL) semanticError("'%s' e uma matriz 2D, precisa de 2 indices!", name);

    if (indexExpr->type == NODE_INT) {
        int idx = indexExpr->intValue;
        int max = symbolTable[node->symbolIndex].arraySize;
        if (idx < 0 || idx >= max) semanticError("Indice/Linha %d fora dos limites de '%s' (Max: %d)!", idx, name, max);
    }
    if (isArr == 2 && index2Expr->type == NODE_INT) {
        int idx2 = index2Expr->intValue;
        int max2 = symbolTable[node->symbolIndex].arrayCols;
        if (idx2 < 0 || idx2 >= max2) semanticError("Coluna %d fora dos limites de '%s' (Max: %d)!", idx2, name, max2);
    }
    return node;
}

AST* parseStatement() {
    if (currentToken.type == T_SWITCH) {
        advance(); 
        if (currentToken.type != T_LPAREN) syntaxError("esperado '(' apos 'switch'");
        advance();
        AST* expr = parseLogical(); 
        if (currentToken.type != T_RPAREN) syntaxError("esperado ')' apos expressao do switch");
        advance();
        if (currentToken.type != T_LBRACE) syntaxError("esperado '{' para iniciar o bloco do switch");
        advance();

        AST* switchNode = createSwitchNode(expr);

        while (currentToken.type != T_RBRACE && currentToken.type != T_EOF) {
            if (currentToken.type == T_CASE) {
                advance();
                AST* caseExpr = parseLogical();
                if (currentToken.type != T_COLON) syntaxError("esperado ':' apos expressao do case");
                advance();
                
                AST* caseBody = malloc(sizeof(AST));
                caseBody->type = NODE_BLOCK;
                caseBody->block.count = 0;
                while (currentToken.type != T_CASE && currentToken.type != T_DEFAULT && currentToken.type != T_RBRACE) {
                    caseBody->block.children[caseBody->block.count++] = parseStatement();
                }
                switchNode->switchStmt.cases[switchNode->switchStmt.caseCount++] = createCaseNode(caseExpr, caseBody);
                
            } else if (currentToken.type == T_DEFAULT) {
                advance();
                if (currentToken.type != T_COLON) syntaxError("esperado ':' apos 'default'");
                advance();
                
                AST* defaultBody = malloc(sizeof(AST));
                defaultBody->type = NODE_BLOCK;
                defaultBody->block.count = 0;
                while (currentToken.type != T_CASE && currentToken.type != T_DEFAULT && currentToken.type != T_RBRACE) {
                    defaultBody->block.children[defaultBody->block.count++] = parseStatement();
                }
                switchNode->switchStmt.cases[switchNode->switchStmt.caseCount++] = createCaseNode(NULL, defaultBody);
            } else {
                advance(); 
            }
        }
        if (currentToken.type != T_RBRACE) syntaxError("esperado '}' para fechar o switch");
        advance();
        return switchNode;
    }

    if (currentToken.type == T_BREAK) {
        advance();
        if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';' apos 'break'");
        advance();
        return createBreakNode();
    }

    if (currentToken.type == T_CONTINUE) {
        advance();
        if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';' apos 'continue'");
        advance();
        return createContinueNode();
    }

    if (currentToken.type == T_WHILE) {
        advance(); 
        if (currentToken.type != T_LPAREN) syntaxError("esperado '(' apos 'while'");
        advance(); 
        AST* cond = parseLogical();
        if (currentToken.type != T_RPAREN) syntaxError("esperado ')' apos condicao do while");
        advance(); 
        
        AST* body = parseBlock();
        
        AST* node = malloc(sizeof(AST));
        node->type = NODE_WHILE;
        node->control.condition = cond;
        node->control.thenBranch = body;
        node->control.elseBranch = NULL;
        return node;
    }

    if (currentToken.type == T_DO) {
        advance(); 
        if (currentToken.type != T_LBRACE) syntaxError("esperado '{' apos 'do'");
        advance();
        
        AST* body = parseBlock(); 
        
        if (currentToken.type != T_WHILE) syntaxError("esperado 'while' no fim do bloco 'do'");
        advance();
        if (currentToken.type != T_LPAREN) syntaxError("esperado '(' apos 'while'");
        advance();
        
        AST* cond = parseLogical();
        
        if (currentToken.type != T_RPAREN) syntaxError("esperado ')' apos condicao");
        advance();
        if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';' no final do do-while");
        advance();
        
        return createDoWhileNode(body, cond);
    }

    if (currentToken.type == T_FOR) {
        advance(); 
        if (currentToken.type != T_LPAREN) syntaxError("esperado '(' apos 'for'");
        advance();
        
        AST* init = parseStatement(); 
        
        AST* cond = parseLogical();
        if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';' apos condicao do for");
        advance();
        
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
            else if (currentToken.type == T_INC) {
                advance(); 
                AST* varNode = createVarNode(varName);
                AST* oneNode = createIntNode(1);
                AST* addNode = createBinOpNode(T_PLUS, varNode, oneNode);
                inc = createAssignNode(varName, addNode);
            } 
            else if (currentToken.type == T_DEC) {
                advance(); 
                AST* varNode = createVarNode(varName);
                AST* oneNode = createIntNode(1);
                AST* subNode = createBinOpNode(T_MINUS, varNode, oneNode);
                inc = createAssignNode(varName, subNode);
            }
            else if (currentToken.type == T_PLUS_ASSIGN || currentToken.type == T_MINUS_ASSIGN || 
                     currentToken.type == T_MUL_ASSIGN || currentToken.type == T_DIV_ASSIGN) {
                TokenType opType;
                if (currentToken.type == T_PLUS_ASSIGN) opType = T_PLUS;
                else if (currentToken.type == T_MINUS_ASSIGN) opType = T_MINUS;
                else if (currentToken.type == T_MUL_ASSIGN) opType = T_MUL;
                else opType = T_DIV;

                advance(); 
                AST* valueNode = parseLogical(); 
                AST* varNode = createVarNode(varName);
                AST* binOpNode = createBinOpNode(opType, varNode, valueNode);
                inc = createAssignNode(varName, binOpNode);
            }
        }
        
        if (currentToken.type != T_RPAREN) syntaxError("esperado ')' no for");
        advance();
        if (currentToken.type != T_LBRACE) syntaxError("esperado '{' para iniciar o bloco do for");
        advance();
        
        AST* body = parseBlock();
        return createForNode(init, cond, inc, body);
    }

    if (currentToken.type == T_LBRACE) {
        advance(); 
        return parseBlock(); 
    }

    if (currentToken.type == T_PRINT) {
        advance(); 
        if (currentToken.type != T_LPAREN) syntaxError("esperado '(' apos 'print'");
        advance();
        AST* expr = parseLogical();
        if (currentToken.type != T_RPAREN) syntaxError("esperado ')' apos expressao do print");
        advance();
        if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';' apos comando print");
        advance();
        return createPrintNode(expr);
    }

    if (currentToken.type == T_READ) {
        advance(); 
        if (currentToken.type != T_LPAREN) syntaxError("esperado '(' apos 'read'");
        advance();
        if (currentToken.type != T_ID) syntaxError("esperado identificador de variavel no read");
        
        char varName[50];
        strcpy(varName, currentToken.lexeme);
        advance();
        
        if (currentToken.type != T_RPAREN) syntaxError("esperado ')' apos variavel do read");
        advance();
        if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';' apos comando read");
        advance();
        return createReadNode(varName);
    }

    if (currentToken.type == T_INT_TYPE || currentToken.type == T_FLOAT_TYPE ||
        currentToken.type == T_BOOL_TYPE || currentToken.type == T_CHAR_TYPE ||
        currentToken.type == T_STRING_TYPE) {

        DataType type;
        if (currentToken.type == T_INT_TYPE) type = TYPE_INT;
        else if (currentToken.type == T_FLOAT_TYPE) type = TYPE_FLOAT;
        else if (currentToken.type == T_BOOL_TYPE) type = TYPE_BOOL;
        else if (currentToken.type == T_CHAR_TYPE) type = TYPE_CHAR;
        else type = TYPE_STRING;

        advance();

        if (currentToken.type != T_ID) syntaxError("esperado nome da variavel na declaracao");

        char name[50];
        strcpy(name, currentToken.lexeme);
        advance(); 

        int isArray = 0;
        int arraySize = 0;
        int arrayCols = 0;

        if (currentToken.type == T_LBRACKET) { 
            advance(); 
            if (currentToken.type != T_INT) syntaxError("o tamanho do vetor/matriz '%s' deve ser um numero inteiro", name);
            arraySize = atoi(currentToken.lexeme);
            advance(); 
            if (currentToken.type != T_RBRACKET) syntaxError("esperado ']'");
            advance(); 
            isArray = 1; 

            if (currentToken.type == T_LBRACKET) {
                advance(); 
                if (currentToken.type != T_INT) syntaxError("o numero de colunas da matriz '%s' deve ser inteiro", name);
                arrayCols = atoi(currentToken.lexeme);
                advance(); 
                if (currentToken.type != T_RBRACKET) syntaxError("esperado ']'");
                advance(); 
                isArray = 2; 
            }
        }

        AST* declNode = createDeclNode(name, type, isArray, arraySize, arrayCols);

        if (currentToken.type == T_ASSIGN) {
            advance(); 

            if (isArray) {
                if (currentToken.type != T_LBRACE) syntaxError("esperado '{' para inicializar o vetor '%s'", name);
                advance(); 

                AST* blockNode = malloc(sizeof(AST));
                blockNode->type = NODE_BLOCK;
                blockNode->block.count = 0;
                blockNode->block.children[blockNode->block.count++] = declNode;

                int currentIndex = 0;
                while (currentToken.type != T_RBRACE) {
                    if (currentIndex >= arraySize) semanticError("Muitos elementos na inicializacao do vetor '%s' (Max: %d)!", name, arraySize);
                    AST* valNode = parseLogical(); 
                    AST* idxNode = createIntNode(currentIndex);
                    
                    AST* assignNode = createArrayAssignNode(name, idxNode, NULL, valNode);
                    blockNode->block.children[blockNode->block.count++] = assignNode;
                    currentIndex++;

                    if (currentToken.type == T_COMMA) advance(); 
                    else if (currentToken.type != T_RBRACE) syntaxError("esperado ',' ou '}' na inicializacao do vetor");
                }
                advance(); 

                if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';' apos inicializacao do vetor");
                advance(); 
                return blockNode;
            } else {
                AST* valueNode = parseLogical(); 
                if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';' apos inicializacao de '%s'", name);
                advance(); 

                AST* assignNode = createAssignNode(name, valueNode);
                AST* blockNode = malloc(sizeof(AST));
                blockNode->type = NODE_BLOCK;
                blockNode->block.count = 2;
                blockNode->block.children[0] = declNode;
                blockNode->block.children[1] = assignNode;
                return blockNode;
            }
        }

        if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';' apos declaracao de '%s'", name);
        advance(); 
        return declNode;
    }

    if (currentToken.type == T_ID) {
        char varName[100];
        strcpy(varName, currentToken.lexeme);
        advance(); 

        AST* arrayIndex = NULL;
        AST* arrayIndex2 = NULL; 
        
        if (currentToken.type == T_LBRACKET) {
            advance(); 
            arrayIndex = parseLogical();
            if (currentToken.type != T_RBRACKET) syntaxError("esperado ']'");
            advance(); 
            
            if (currentToken.type == T_LBRACKET) {
                advance();
                arrayIndex2 = parseLogical();
                if (currentToken.type != T_RBRACKET) syntaxError("esperado ']'");
                advance();
            }
        }

        if (currentToken.type == T_ASSIGN) {
            advance();
            AST* value = parseLogical();
            if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';'");
            advance();
            return arrayIndex ? createArrayAssignNode(varName, arrayIndex, arrayIndex2, value) : createAssignNode(varName, value);
        }
        else if (currentToken.type == T_INC) {
            advance();
            if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';'");
            advance();
            AST* varNode = arrayIndex ? createArrayAccessNode(varName, arrayIndex, arrayIndex2) : createVarNode(varName);
            AST* oneNode = createIntNode(1);
            AST* addNode = createBinOpNode(T_PLUS, varNode, oneNode);
            return arrayIndex ? createArrayAssignNode(varName, arrayIndex, arrayIndex2, addNode) : createAssignNode(varName, addNode);
        }
        else if (currentToken.type == T_DEC) {
            advance();
            if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';'");
            advance();
            AST* varNode = arrayIndex ? createArrayAccessNode(varName, arrayIndex, arrayIndex2) : createVarNode(varName);
            AST* oneNode = createIntNode(1);
            AST* subNode = createBinOpNode(T_MINUS, varNode, oneNode);
            return arrayIndex ? createArrayAssignNode(varName, arrayIndex, arrayIndex2, subNode) : createAssignNode(varName, subNode);
        }
        else if (currentToken.type == T_PLUS_ASSIGN || currentToken.type == T_MINUS_ASSIGN || 
                 currentToken.type == T_MUL_ASSIGN || currentToken.type == T_DIV_ASSIGN) {
            TokenType opType;
            if (currentToken.type == T_PLUS_ASSIGN) opType = T_PLUS;
            else if (currentToken.type == T_MINUS_ASSIGN) opType = T_MINUS;
            else if (currentToken.type == T_MUL_ASSIGN) opType = T_MUL;
            else opType = T_DIV;

            advance();
            AST* valueNode = parseLogical();
            if (currentToken.type != T_SEMICOLON) syntaxError("esperado ';'");
            advance();
            
            AST* varNode = arrayIndex ? createArrayAccessNode(varName, arrayIndex, arrayIndex2) : createVarNode(varName);
            AST* binOpNode = createBinOpNode(opType, varNode, valueNode);
            return arrayIndex ? createArrayAssignNode(varName, arrayIndex, arrayIndex2, binOpNode) : createAssignNode(varName, binOpNode);
        } 
        else {
            syntaxError("esperado '=', '++', '--', '+=', '-=', '*=' ou '/=' apos o identificador '%s'", varName);
        }
    }

    while (currentToken.type == T_SEMICOLON) {
        advance();
    }

    syntaxError("comando invalido (nao se pode comecar com o token '%s')", currentToken.lexeme);
    return NULL;
}

AST* parseBlock() {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_BLOCK;
    node->block.count = 0;

    currentScope++; 

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
        syntaxError("esperado '}' para fechar o bloco");
    }

    for (int i = 0; i < varCount; i++) {
        if (symbolTable[i].scope == currentScope && symbolTable[i].isActive) {
            
            // NOVO: O puxão de orelhas se a variável foi esquecida
            if (symbolTable[i].isUsed == 0) {
                printf("\n[!] AVISO (Linha %d): Variavel '%s' declarada, mas nunca utilizada.\n", 
                        symbolTable[i].line, symbolTable[i].name);
            }
            
            symbolTable[i].isActive = 0; // Fica invisível para os blocos de fora
        }
    }
    
    currentScope--; 
    return node;
}

AST* parseProgram() {
    advance(); 

    AST* programNode = malloc(sizeof(AST));
    programNode->type = NODE_BLOCK;
    programNode->block.count = 0;

    while (currentToken.type != T_EOF) {
        
        // PONTO DE ATERRAGEM DO PANIC MODE
        if (setjmp(panicBuffer) != 0) {
            // Se caiu aqui, é porque houve um erro. Vamos Sincronizar!
            currentScope = 0; // Prevenção de segurança de escopo
            
            // Engole todos os tokens até encontrar um ponto e vírgula ou uma chave
            while (currentToken.type != T_SEMICOLON && currentToken.type != T_RBRACE && currentToken.type != T_EOF) {
                advance();
            }
            if (currentToken.type == T_SEMICOLON || currentToken.type == T_RBRACE) {
                advance(); // Consome o delimitador para recomeçar limpo
            }
            continue; // Tenta compilar a próxima linha!
        }

        if (currentToken.type == T_SEMICOLON) {
            advance();
            continue;
        }

        if (currentToken.lexeme[0] == '\0') {
            advance();
            continue;
        }

        if (currentToken.type == T_INT || currentToken.type == T_FLOAT || currentToken.type == T_LPAREN) {
            AST* expr = parseLogical();
            if (currentToken.type == T_SEMICOLON) advance();
            programNode->block.children[programNode->block.count++] = expr;
            continue;
        }
        
        AST* stmt = parseStatement();
        if (stmt != NULL) {
            programNode->block.children[programNode->block.count++] = stmt;
        }
    }

    for (int i = 0; i < varCount; i++) {
        if (symbolTable[i].scope == 0 && symbolTable[i].isActive && symbolTable[i].isUsed == 0) {
            printf("\n[!] AVISO (Linha %d): Variavel '%s' declarada, mas nunca utilizada.\n", 
                    symbolTable[i].line, symbolTable[i].name);
        }
    }

    return programNode;
}

AST* createCharNode(char value) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_CHAR;
    node->charValue = value;
    return node;
}

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
    
    node->symbolIndex = findVar(varName);
    if (node->symbolIndex == -1) semanticError("Variavel '%s' nao declarada na leitura!", varName);
    symbolTable[node->symbolIndex].isInitialized = 1;
    return node;
}

float evaluate(AST* node) {
    if (node->type == NODE_ARRAY_ACCESS || node->type == NODE_ARRAY_ASSIGN) return 0;
    if (node == NULL) return 0;

    if (node->type == NODE_SWITCH) {
        if (node->switchStmt.expr) evaluate(node->switchStmt.expr);
        for(int i = 0; i < node->switchStmt.caseCount; i++) {
            if(node->switchStmt.cases[i]->caseStmt.matchExpr) evaluate(node->switchStmt.cases[i]->caseStmt.matchExpr);
            if(node->switchStmt.cases[i]->caseStmt.body) evaluate(node->switchStmt.cases[i]->caseStmt.body);
        }
        return 0;
    }

    if (node->type == NODE_DECL) return 0;

    if (node->type == NODE_BLOCK) {
        float res = 0;
        for (int i = 0; i < node->block.count; i++) res = evaluate(node->block.children[i]);
        return res; 
    }

     if (node->type == 6 || node->type == NODE_IF) {
        if (node->control.condition) evaluate(node->control.condition);
        if (node->control.thenBranch) evaluate(node->control.thenBranch);
        if (node->control.elseBranch) evaluate(node->control.elseBranch);
        return 0;
    }

    if (node->type == NODE_ASSIGN) {
        int pos = node->symbolIndex; 
        if (symbolTable[pos].type == TYPE_STRING) {
            if (node->assign.value->type == NODE_STRING) strcpy(symbolTable[pos].value.s, node->assign.value->stringValue);
            return 0; 
        }

        if (symbolTable[pos].type == TYPE_CHAR && node->assign.value->type == NODE_STRING) {
            semanticError("Incompatibilidade de tipos! Nao se pode atribuir uma STRING (Texto) a uma variavel CHAR.");
        }

        float val = evaluate(node->assign.value);

        if (symbolTable[pos].type == TYPE_INT) symbolTable[pos].value.i = (int)val;
        else if (symbolTable[pos].type == TYPE_FLOAT) symbolTable[pos].value.f = (float)val;
        else if (symbolTable[pos].type == TYPE_BOOL) symbolTable[pos].value.i = (val != 0);
        else if (symbolTable[pos].type == TYPE_CHAR) symbolTable[pos].value.c = (char)val;

        return val;
    }

    if (node->type == NODE_INT) return node->intValue;
    if (node->type == NODE_FLOAT) return node->floatValue;
    if (node->type == NODE_CHAR) return node->charValue;
    if (node->type == NODE_BOOL) return node->boolValue;

    if (node->type == NODE_UNOP) {
        float val = evaluate(node->unop.expr);
        switch (node->unop.op) {
            case T_NOT: return !val;
            default: semanticError("Operador unario desconhecido");
        }
    }

    if (node->type == NODE_BINOP) {
        if (node->binop.left->type == NODE_STRING || node->binop.right->type == NODE_STRING) {
            semanticError("Operacao invalida! Nao e possivel realizar operacoes aritmeticas com STRINGS.");
        }

        float left = evaluate(node->binop.left);
        float right = evaluate(node->binop.right);

        switch (node->binop.op) {
            case T_PLUS: return left + right;
            case T_MINUS: return left - right;
            case T_MUL: return left * right;
            case T_DIV: return left / right;
            case T_GT: return left > right;
            case T_LT: return left < right;
            case T_GTE: return left >= right;
            case T_LTE: return left <= right;
            case T_EQ: return left == right;
            case T_NEQ: return left != right;
            case T_AND: return (left != 0 && right != 0);
            case T_OR:  return (left != 0 || right != 0);
            default: semanticError("Operador desconhecido");
        }
    }

    if (node->type == NODE_CAST) {
        float val = evaluate(node->cast.expr);
        if (node->cast.type == TYPE_INT) return (int)val;
        if (node->cast.type == TYPE_FLOAT) return (float)val;
        if (node->cast.type == TYPE_BOOL) return (val != 0);
    }

    if (node->type == NODE_VAR) {
        int pos = node->symbolIndex;
        if (symbolTable[pos].type == TYPE_INT) return (float)symbolTable[pos].value.i;
        else return symbolTable[pos].value.f;
    }

     if (node->type == NODE_WHILE) {
        if (node->control.condition) evaluate(node->control.condition);
        if (node->control.thenBranch) evaluate(node->control.thenBranch);
        return 0;
    }

    if (node->type == NODE_STRING) return 0; 
    if (node->type == NODE_PRINT) { evaluate(node->printStmt.expr); return 0; }
    if (node->type == NODE_READ) return 0; 

    if (node->type == NODE_FOR) {
        if(node->forLoop.init) evaluate(node->forLoop.init);
        if(node->forLoop.condition) evaluate(node->forLoop.condition);
        if(node->forLoop.increment) evaluate(node->forLoop.increment);
        if(node->forLoop.body) evaluate(node->forLoop.body);
        return 0;
    }

    if (node->type == NODE_DO_WHILE) {
        if(node->doWhileLoop.body) evaluate(node->doWhileLoop.body);
        if(node->doWhileLoop.condition) evaluate(node->doWhileLoop.condition);
        return 0;
    }

    if (node->type == NODE_BREAK || node->type == NODE_CONTINUE) return 0;
    
    semanticError("Erro inesperado na avaliacao (tipo: %d)", node->type);
    return 0;
}

void printIndent(int indent) {
    for(int i = 0; i < indent; i++) printf("    ");
}

void generateC_expr(AST* node) {
    if (!node) return;

    switch(node->type) {
        case NODE_STRING: printf("\"%s\"", node->stringValue); break;
        case NODE_INT: printf("%d", node->intValue); break;
        case NODE_FLOAT: printf("%.1f", node->floatValue); break;
        case NODE_CHAR: printf("'%c'", node->charValue); break;
        case NODE_BOOL: printf("%s", node->boolValue ? "true" : "false"); break;
        case NODE_VAR: printf("%s", node->varName); break;
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
                case T_GTE:   printf(" >= "); break; 
                case T_LTE:   printf(" <= "); break;
                case T_EQ:    printf(" == "); break;
                case T_NEQ:   printf(" != "); break;
                case T_AND:   printf(" && "); break;
                case T_OR:    printf(" || "); break;
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
                case TYPE_STRING: break;
            }
            generateC_expr(node->cast.expr);
            printf(")");
            break;
        default: break;
    }
}

void generateC(AST* node, int indent) {
    if (node == NULL) return;

    switch(node->type) {
        case NODE_BLOCK:
            for(int i = 0; i < node->block.count; i++) generateC(node->block.children[i], indent);
            break;

        case NODE_DECL:
            printIndent(indent);
            switch(node->decl.type) {
                case TYPE_INT:   printf("int %s;\n", node->decl.varName); break;
                case TYPE_FLOAT: printf("float %s;\n", node->decl.varName); break;
                case TYPE_CHAR:  printf("char %s;\n", node->decl.varName); break;
                case TYPE_BOOL:  printf("bool %s;\n", node->decl.varName); break;
                default: break;
            }
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
            DataType tr = symbolTable[pos].type;
            if (tr == TYPE_INT || tr == TYPE_BOOL) printf("scanf(\"%%d\", &%s);\n", node->readStmt.varName);
            else if (tr == TYPE_FLOAT) printf("scanf(\"%%f\", &%s);\n", node->readStmt.varName);
            else if (tr == TYPE_CHAR) printf("scanf(\" %%c\", &%s);\n", node->readStmt.varName);
            break;
        
        case NODE_ASSIGN:
            printIndent(indent);
            printf("// TAC: %s = ...\n", node->assign.varName); 
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
            generateC(node->control.thenBranch, indent + 1);
            printIndent(indent);
            printf("}");
            if (node->control.elseBranch) {
                printf(" else {\n");
                generateC(node->control.elseBranch, indent + 1);
                printIndent(indent);
                printf("}\n");
            } else {
                printf("\n");
            }
            break;

        case NODE_WHILE:
            printIndent(indent);
            printf("while (");
            generateC_expr(node->control.condition);
            printf(") {\n");
            if (node->control.thenBranch) generateC(node->control.thenBranch, indent + 1);
            printIndent(indent);
            printf("}\n");
            break;

        case NODE_SWITCH:
            printIndent(indent);
            printf("switch (");
            generateC_expr(node->switchStmt.expr);
            printf(") {\n");
            for(int i = 0; i < node->switchStmt.caseCount; i++) {
                AST* c = node->switchStmt.cases[i];
                printIndent(indent + 1);
                if (c->caseStmt.matchExpr) {
                    printf("case ");
                    generateC_expr(c->caseStmt.matchExpr);
                    printf(":\n");
                } else {
                    printf("default:\n");
                }
                
                if (c->caseStmt.body && c->caseStmt.body->type == NODE_BLOCK) {
                    for(int b = 0; b < c->caseStmt.body->block.count; b++) {
                        generateC(c->caseStmt.body->block.children[b], indent + 2);
                    }
                }
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
            if (node->doWhileLoop.body) generateC(node->doWhileLoop.body, indent + 1);
            printIndent(indent);
            printf("} while (");
            generateC_expr(node->doWhileLoop.condition);
            printf(");\n");
            break;

        case NODE_FOR:
            printIndent(indent);
            printf("for (");
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
            
            if (node->forLoop.body) generateC(node->forLoop.body, indent + 1);
            printIndent(indent);
            printf("}\n");
            break; 

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

        default: break;
    }
}

int getStringSize(int index) {
    if (index < varCount) {
        int len = strlen(symbolTable[index].value.s);
        if (len > 0) return len + 1; 
    }
    return 255; 
}

int isSymbolArray(int index) {
    if (index < varCount) return symbolTable[index].isArray;
    return 0;
}
int getSymbolArraySize(int index) {
    if (index < varCount) return symbolTable[index].arraySize;
    return 0;
}
int getSymbolArrayCols(int index) {
    if (index < varCount) return symbolTable[index].arrayCols;
    return 0;
}