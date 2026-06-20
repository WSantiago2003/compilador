#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int tacPrint = 1;
int varCount = 0;
int currentScope = 0;
int varNum = 0;
// Gerador de Labels (L0, L1, L2...)
int labelCount = 0;
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

// Pilha para armazenar os saltos de break e continue
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


Token currentToken;

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

    if (node->type == NODE_VAR) {
        return symbolTable[node->symbolIndex].type; // Busca direta e sem erro!
    }

    if (node->type == NODE_STRING) {
        return TYPE_STRING;
    }

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

    // --- GERADOR DE TAC: CONTROLE DE FLUXO ---
    if (node->type == NODE_IF) {
        char* cond = generateTAC(node->control.condition);
        char* lElse = newLabel();
        char* lEnd = newLabel();

        // 1. Desvio condicional inicial
        if (tacPrint == 1) {
            printf("ifFalse %s goto %s;\n", cond, node->control.elseBranch ? lElse : lEnd);
        } else if (tacPrint == 2) { 
            printf("    if (!%s) goto %s;\n", cond, node->control.elseBranch ? lElse : lEnd);
        }
        
        // 2. Corpo do THEN
        generateTAC(node->control.thenBranch);
        
        // 3. Tratamento do ELSE (se houver)
        if (node->control.elseBranch) {
            if (tacPrint == 1) {
                printf("goto %s;\n", lEnd);
                printf("%s:\n", lElse);
            } else if (tacPrint == 2) {
                printf("    goto %s;\n", lEnd); // Salta o bloco else se o then já executou
                printf("    %s:\n", lElse);    // Rótulo do Else indentado
            }
            
            generateTAC(node->control.elseBranch);
        } else {
            // Se NÃO houver else, o rótulo de escape do ifFalse precisa ser impresso aqui para o modo TAC puro
            if (tacPrint == 1) {
                printf("%s:\n", lElse);
            } else if (tacPrint == 2) {
                printf("    %s:\n", lElse);
            }
        }
        
        // 4. Rótulo de FIM do bloco IF completo
        if (tacPrint == 1) {
            printf("%s:\n", lEnd);
        } else if (tacPrint == 2) {
            printf("    %s:\n", lEnd); // Rótulo de Fim indentado
        }
        
        return NULL;
    }

    if (node->type == NODE_WHILE) {
        char* lStart = newLabel();
        char* lEnd = newLabel();
        pushLoop(lStart, lEnd); // break vai pro lEnd, continue vai pro lStart

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
        pushLoop(lCond, lEnd); // no do/while, o continue pula para a avaliação da condição!

        if (tacPrint) printf("%s:\n", lStart);
        if (node->doWhileLoop.body) generateTAC(node->doWhileLoop.body);
        
        if (tacPrint) printf("%s:\n", lCond);
        char* cond = generateTAC(node->doWhileLoop.condition);
        if (tacPrint) printf("if %s goto %s;\n", cond, lStart); // Se verdadeiro, repete
        
        if (tacPrint) printf("%s:\n", lEnd);
        
        popLoop();
        return NULL;
    }

    if (node->type == NODE_FOR) {
        char* lStart = newLabel();
        char* lInc = newLabel();
        char* lEnd = newLabel();
        
        // 1. Inicialização do FOR (ex: i = 1)
        if (node->forLoop.init) generateTAC(node->forLoop.init);
        
        // 2. Rótulo de início do laço (L0:)
        if (tacPrint == 1) {
            printf("%s:\n", lStart);
        } else if (tacPrint == 2) {
            printf("    %s:\n", lStart); // Indentado para o código C
        }
        
        // 3. Avaliação da condição (ex: i <= 10)
        if (node->forLoop.condition) {
            char* cond = generateTAC(node->forLoop.condition);
            if (tacPrint == 1) {
                printf("ifFalse %s goto %s;\n", cond, lEnd);
            } else if (tacPrint == 2) {
                printf("    if (!%s) goto %s;\n", cond, lEnd); 
            }
        }

        // 4. Execução do corpo do laço
        pushLoop(lInc, lEnd);
        if (node->forLoop.body) generateTAC(node->forLoop.body);
        
        // 5. Rótulo de incremento (L1:)
        if (tacPrint == 1) {
            printf("%s:\n", lInc);
        } else if (tacPrint == 2) {
            printf("    %s:\n", lInc); // Indentado para o código C
        }
        
        // 6. Execução do incremento (ex: i = i + 1)
        if (node->forLoop.increment) generateTAC(node->forLoop.increment);
        
        // 7. Salto de retorno para o início do laço
        if (tacPrint == 1) {
            printf("goto %s;\n", lStart);
        } else if (tacPrint == 2) {
            printf("    goto %s;\n", lStart); 
        }
        
        // 8. Rótulo de fim do laço (L2:)
        if (tacPrint == 1) {
            printf("%s:\n", lEnd);
        } else if (tacPrint == 2) {
            printf("    %s:\n", lEnd); // Indentado para o código C
        }
        
        popLoop();
        return NULL;
    }

    if (node->type == NODE_SWITCH) {
        char* val = generateTAC(node->switchStmt.expr);
        char* lEnd = newLabel();
        char* caseLabels[20];
        char* lDefault = lEnd;

        // 1. Avalia todas as condições primeiro (como um Assembly real)
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

        // 2. Empilha o label de fim para o BREAK funcionar
        // Se houver um continue, ele repassa pro laço de fora
        pushLoop(loopDepth > 0 ? loopStartStack[loopDepth - 1] : "L_DUMMY", lEnd); 
        
        // 3. Imprime os blocos de código
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
        // Checagem se o nó for um texto literal diretamente
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

            if (type == TYPE_CHAR) {
                printf("    printf(\"%%c\\n\", %s);\n", expr);
            } else if (type == TYPE_STRING) {
                printf("    printf(\"%%s\\n\", %s);\n", expr); // Imprime a string!
            } else {
                printf("    printf(\"%%d\\n\", %s);\n", expr);
            }
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
                // Lê strings (inclusive com espaços) respeitando o limite de segurança do vetor
                printf("    scanf(\" %%254[^\\n]\", %s);\n", varTemp); 
                // Mede o que o usuário digitou e atualiza a variável companheira na hora!
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
        return getVarTempByIndex(node->symbolIndex);
    }

    if (node->type == NODE_UNOP) {
        char* expr = generateTAC(node->unop.expr);
        char* temp = newTemp();
        if (tacPrint) {
            if (node->unop.op == T_NOT) {
                printf("%s = !%s;\n", temp, expr);
            }
        }
        return temp;
    }

    if (node->type == NODE_BINOP) {
        DataType leftType = getNodeType(node->binop.left);
        DataType rightType = getNodeType(node->binop.right);

        char* left = generateTAC(node->binop.left);
        char* right = generateTAC(node->binop.right);

        // conversão implícita: int -> float (Lado Esquerdo)
        if (leftType == TYPE_INT && rightType == TYPE_FLOAT) {
            char* castTemp = newTemp();
            int castNum = atoi(castTemp + 1);

            tempTypes[castNum] = TYPE_FLOAT;

            if (tacPrint == 1) {
                printf("%s = (float) %s;\n", castTemp, left);
            } else if (tacPrint == 2) {
                printf("    %s = (float) %s;\n", castTemp, left); // Formato C indentado
            }

            left = castTemp;
        }

        // conversão implícita: float -> int (Lado Direito)
        if (leftType == TYPE_FLOAT && rightType == TYPE_INT) {
            char* castTemp = newTemp();
            int castNum = atoi(castTemp + 1);

            tempTypes[castNum] = TYPE_FLOAT;

            if (tacPrint == 1) {
                printf("%s = (float) %s;\n", castTemp, right);
            } else if (tacPrint == 2) {
                printf("    %s = (float) %s;\n", castTemp, right); // Formato C indentado
            }

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

        // --- PRINT FINAL DA OPERAÇÃO BINÁRIA ---
        if (tacPrint == 1) {
            printf("%s = %s %s %s;\n", temp, left, op, right);
        } else if (tacPrint == 2) {
            printf("    %s = %s %s %s;\n", temp, left, op, right); // Formato C indentado
        }

        return temp;
    }

   if (node->type == NODE_ASSIGN) {
        char* varTemp = getVarTempByIndex(node->symbolIndex);
        AST* value = node->assign.value;

        // TRATAMENTO DE STRINGS (Tempo de Compilação - Tamanho Fixo)
        if (symbolTable[node->symbolIndex].type == TYPE_STRING) {
            char* val = generateTAC(value);
            // Pega o tamanho real da palavra (sem contar o \0)
            int strLen = getStringSize(node->symbolIndex) - 1; 

            if (tacPrint == 1) {
                printf("%s = %s;\n", varTemp, val);
                printf("%s_len = %d;\n", varTemp, strLen); // Salva o tamanho no TAC
            } else if (tacPrint == 2) {
                printf("    strcpy(%s, %s);\n", varTemp, val);
                printf("    %s_len = %d;\n", varTemp, strLen); // Salva o tamanho no C
            }
            return NULL;
        }
        // atribuição simples: NÃO cria temporário extra
        if (value->type == NODE_INT) {
            tempTypes[varNum] = TYPE_INT;

            if (tacPrint == 1) {
                printf("%s = %d;\n", varTemp, value->intValue);
            } else if (tacPrint == 2) {
                printf("    %s = %d;\n", varTemp, value->intValue); // Formato C indentado
            }

            return NULL;
        }

        if (value->type == NODE_FLOAT) {
            tempTypes[varNum] = TYPE_FLOAT;

            if (tacPrint == 1) {
                printf("%s = %.1f;\n", varTemp, value->floatValue);
            } else if (tacPrint == 2) {
                printf("    %s = %.1f;\n", varTemp, value->floatValue); // Formato C indentado
            }

            return NULL;
        }

        if (value->type == NODE_CHAR) {
            tempTypes[varNum] = TYPE_CHAR;

            if (tacPrint == 1) {
                printf("%s = '%c';\n", varTemp, value->charValue);
            } else if (tacPrint == 2) {
                printf("    %s = '%c';\n", varTemp, value->charValue); // Formato C indentado
            }

            return NULL;
        }

        if (value->type == NODE_BOOL) {
            tempTypes[varNum] = TYPE_INT;

            if (tacPrint == 1) {
                printf("%s = %d;\n", varTemp, value->boolValue);
            } else if (tacPrint == 2) {
                printf("    %s = %d;\n", varTemp, value->boolValue); // Formato C indentado
            }

            return NULL;
        }

        // expressão composta: gera temporários normalmente
        char* val = generateTAC(value);

        if (tacPrint == 1) {
            printf("%s = %s;\n", varTemp, val);
        } else if (tacPrint == 2) {
            printf("    %s = %s;\n", varTemp, val); // Formato C indentado
        }

        return NULL;
    }

    if (node->type == NODE_DECL) {
        return 0;
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

    if (node->type == NODE_UNOP) {
        return countTemps(node->unop.expr) + 1;
    }

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

    if (node->type == NODE_IF) {
        int total = countTemps(node->control.condition);
        total += countTemps(node->control.thenBranch);
        if (node->control.elseBranch) {
            total += countTemps(node->control.elseBranch);
        }
        return total;
    }

    if (node->type == NODE_WHILE) {
        return countTemps(node->control.condition) + countTemps(node->control.thenBranch);
    }

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
                total += countTemps(node->switchStmt.cases[i]->caseStmt.matchExpr);
                total += 1; // +1 porque o TAC vai fazer: t_cmp = expr == case_expr
            }
            if(node->switchStmt.cases[i]->caseStmt.body) {
                total += countTemps(node->switchStmt.cases[i]->caseStmt.body);
            }
        }
        return total;
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
    
    node->symbolIndex = findVar(name); // Resolve qual é o "x" correto agora
    if (node->symbolIndex == -1) {
        printf("Erro Semantico: Variavel '%s' nao declarada!\n", name);
        exit(1);
    }
    return node;
}

AST* createAssignNode(char* name, AST* value) {
    AST* node = malloc(sizeof(AST));
    node->type = NODE_ASSIGN;
    strcpy(node->assign.varName, name);
    node->assign.value = value;
    
    node->symbolIndex = findVar(name);
    if (node->symbolIndex == -1) {
        printf("Erro Semantico: Variavel '%s' nao declarada na atribuicao!\n", name);
        exit(1);
    }
    return node;
}

int findVar(char* name) {
    // Busca de trás para frente (prioriza o escopo mais interno)
    for (int i = varCount - 1; i >= 0; i--) {
        // Só considera a variável se ela estiver ativa
        if (symbolTable[i].isActive && strcmp(symbolTable[i].name, name) == 0) {
            return i;
        }
    }
    return -1; // Não encontrou em nenhum escopo visível
}

int addSymbol(char* name, DataType type) {
    // Verifica se a variável já existe NO MESMO ESCOPO
    for (int i = varCount - 1; i >= 0; i--) {
        if (!symbolTable[i].isActive) continue; // Ignora variáveis de blocos já fechados
        if (symbolTable[i].scope < currentScope) break; // Saiu do escopo atual, pode parar
        
        if (strcmp(symbolTable[i].name, name) == 0) {
            printf("Erro: Variavel '%s' ja declarada neste escopo!\n", name);
            exit(1);
        }
    }

    if (varCount < MAX_VARS) {
        strcpy(symbolTable[varCount].name, name);
        symbolTable[varCount].type = type;
        symbolTable[varCount].scope = currentScope;
        symbolTable[varCount].isActive = 1; // Marca como ativa
        
        // Inicializa com zero
        if (type == TYPE_INT) symbolTable[varCount].value.i = 0;
        else if (type == TYPE_FLOAT) symbolTable[varCount].value.f = 0.0;
        else if (type == TYPE_CHAR) symbolTable[varCount].value.c = '\0';
        else if (type == TYPE_STRING) strcpy(symbolTable[varCount].value.s, "");
        
        varCount++;
        return varCount - 1; // Retorna o ID do TAC que acabou de ser criado
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
    
    // Adiciona e já salva o índice na AST!
    node->symbolIndex = addSymbol(name, type);
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

    // Lê o operador unário NOT (!)
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
            currentToken.type == T_GTE || currentToken.type == T_LTE ||
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
    AST* node = parseRelational(); // ou parseExpression, dependendo de como está seu código base

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

//parser do IF
AST* parseIf() {

    advance(); // pula "if"

    if (currentToken.type != T_LPAREN) {
        printf("Erro: esperado '('\n");
        exit(1);
    }

    advance();
    AST* condition = parseLogical();

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

        // O truque: se vier um 'if' logo a seguir, chama a si mesmo!
        if (currentToken.type == T_IF) {
            elseBranch = parseIf(); 
        } else {
            if (currentToken.type != T_LBRACE) {
                printf("Erro: esperado '{' apos else\n");
                exit(1);
            }
            advance(); // entra no bloco
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

AST* parseStatement() {

    // PARSING: SWITCH
    if (currentToken.type == T_SWITCH) {
        advance(); 
        if (currentToken.type != T_LPAREN) { printf("Erro: esperado '('\n"); exit(1); }
        advance();
        AST* expr = parseLogical(); 
        if (currentToken.type != T_RPAREN) { printf("Erro: esperado ')'\n"); exit(1); }
        advance();
        if (currentToken.type != T_LBRACE) { printf("Erro: esperado '{'\n"); exit(1); }
        advance();

        AST* switchNode = createSwitchNode(expr);

        // Lê os cases até fechar a chave
        while (currentToken.type != T_RBRACE && currentToken.type != T_EOF) {
            if (currentToken.type == T_CASE) {
                advance();
                AST* caseExpr = parseLogical();
                if (currentToken.type != T_COLON) { printf("Erro: esperado ':'\n"); exit(1); }
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
                if (currentToken.type != T_COLON) { printf("Erro: esperado ':'\n"); exit(1); }
                advance();
                
                AST* defaultBody = malloc(sizeof(AST));
                defaultBody->type = NODE_BLOCK;
                defaultBody->block.count = 0;
                while (currentToken.type != T_CASE && currentToken.type != T_DEFAULT && currentToken.type != T_RBRACE) {
                    defaultBody->block.children[defaultBody->block.count++] = parseStatement();
                }
                switchNode->switchStmt.cases[switchNode->switchStmt.caseCount++] = createCaseNode(NULL, defaultBody);
            } else {
                advance(); // ignora lixo para não travar
            }
        }
        if (currentToken.type != T_RBRACE) { printf("Erro: esperado '}' no switch\n"); exit(1); }
        advance();
        return switchNode;
    }

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
        
        AST* cond = parseLogical();
        
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
        AST* cond = parseLogical();
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

    // Se encontrar um bloco avulso (usado para escopos locais restritos)
    if (currentToken.type == T_LBRACE) {
        advance(); // Consome a chave '{'
        return parseBlock(); // Lê tudo lá dentro até o '}'
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
        currentToken.type == T_CHAR_TYPE ||
        currentToken.type == T_STRING_TYPE) {

        DataType type;

        if (currentToken.type == T_INT_TYPE) type = TYPE_INT;
        else if (currentToken.type == T_FLOAT_TYPE) type = TYPE_FLOAT;
        else if (currentToken.type == T_BOOL_TYPE) type = TYPE_BOOL;
        else if (currentToken.type == T_CHAR_TYPE) type = TYPE_CHAR;
        else type = TYPE_STRING;

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
        advance(); // pula T_WHILE
        if (currentToken.type != T_LPAREN) { printf("Erro: esperado '('\n"); exit(1); }
        advance(); // pula '('
        AST* cond = parseLogical();
        if (currentToken.type != T_RPAREN) { printf("Erro: esperado ')'\n"); exit(1); }
        advance(); // pula ')'
        
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

    currentScope++; // Entra em um novo escopo

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

    // Desativa as variáveis DESTE escopo
    for (int i = 0; i < varCount; i++) {
        if (symbolTable[i].scope == currentScope) {
            symbolTable[i].isActive = 0; // Fica invisível para os blocos de fora
        }
    }
    
    currentScope--; // Volta para o escopo anterior
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
    
    node->symbolIndex = findVar(varName);
    if (node->symbolIndex == -1) {
        printf("Erro: Variavel '%s' nao declarada na leitura!\n", varName);
        exit(1);
    }
    return node;
}

//funcao de avaliçao da arvore AST
float evaluate(AST* node) {

    if (node == NULL) return 0;
    //printf("DEBUG: Executando %s\n", nodeTypeToString(node->type));

    if (node->type == NODE_SWITCH) {
        if (node->switchStmt.expr) evaluate(node->switchStmt.expr);
        for(int i = 0; i < node->switchStmt.caseCount; i++) {
            if(node->switchStmt.cases[i]->caseStmt.matchExpr) {
                evaluate(node->switchStmt.cases[i]->caseStmt.matchExpr);
            }
            if(node->switchStmt.cases[i]->caseStmt.body) {
                evaluate(node->switchStmt.cases[i]->caseStmt.body);
            }
        }
        return 0;
    }

    if (node->type == NODE_DECL) {
        return 0;
    }

    //percorre o bloco
    if (node->type == NODE_BLOCK) {
        float res = 0;
        for (int i = 0; i < node->block.count; i++) {
            res = evaluate(node->block.children[i]);
        }
        return res; // Não precisa mais mexer em escopo aqui
    }

    //if
     if (node->type == 6 || node->type == NODE_IF) {
        if (node->control.condition) evaluate(node->control.condition);
        if (node->control.thenBranch) evaluate(node->control.thenBranch);
        if (node->control.elseBranch) evaluate(node->control.elseBranch);
        return 0;
    }

    // atribuição
    if (node->type == NODE_ASSIGN) {
        int pos = node->symbolIndex; 

        if (symbolTable[pos].type == TYPE_STRING) {
            // NOVO: Na passada silenciosa, pega a palavra e salva na tabela!
            if (node->assign.value->type == NODE_STRING) {
                strcpy(symbolTable[pos].value.s, node->assign.value->stringValue);
            }
            return 0; 
        }

        // Checagem Semântica (Incompatibilidade)
        if (symbolTable[pos].type == TYPE_CHAR && node->assign.value->type == NODE_STRING) {
            printf("Erro Semantico: Incompatibilidade de tipos! Nao se pode atribuir uma STRING (Texto) a uma variavel CHAR.\n");
            exit(1);
        }

        float val = evaluate(node->assign.value);

        if (symbolTable[pos].type == TYPE_INT) symbolTable[pos].value.i = (int)val;
        else if (symbolTable[pos].type == TYPE_FLOAT) symbolTable[pos].value.f = (float)val;
        else if (symbolTable[pos].type == TYPE_BOOL) symbolTable[pos].value.i = (val != 0);
        else if (symbolTable[pos].type == TYPE_CHAR) symbolTable[pos].value.c = (char)val;

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

        if (node->binop.left->type == NODE_STRING || node->binop.right->type == NODE_STRING) {
            printf("Erro Semantico: Operacao invalida! Nao e possivel realizar operacoes aritmeticas ou relacionais com STRINGS (Texto).\n");
            exit(1);
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
        int pos = node->symbolIndex;
        if (symbolTable[pos].type == TYPE_INT) {
            return (float)symbolTable[pos].value.i;
        } else {
            return symbolTable[pos].value.f;
        }
    }

    

     if (node->type == NODE_WHILE) {
        if (node->control.condition) evaluate(node->control.condition);
        if (node->control.thenBranch) evaluate(node->control.thenBranch);
        return 0;
    }

    // --- NOVOS NÓS DE I/O E STRINGS ---
    if (node->type == NODE_STRING) {
        return 0; 
    }

    if (node->type == NODE_PRINT) {
        evaluate(node->printStmt.expr); // Apenas avalia a expressão de dentro para checar se as variáveis existem
        return 0;
    }

    if (node->type == NODE_READ) {
        return 0; // O erro de não declaração já foi pego no Parser
    }

    // --- NOVOS NÓS DE LAÇOS (FOR, DO/WHILE, BREAK, CONTINUE) ---
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

    if (node->type == NODE_BREAK || node->type == NODE_CONTINUE) {
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
        case NODE_STRING:
            printf("\"%s\"", node->stringValue);
            break;
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
                case T_GTE:   printf(" >= "); break; 
                case T_LTE:   printf(" <= "); break;
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
                case TYPE_STRING: break;
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
    if (node == NULL) return;

    switch(node->type) {
        case NODE_BLOCK:
            for(int i = 0; i < node->block.count; i++) {
                generateC(node->block.children[i], indent);
            }
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
            printf("// TAC: %s = ...\n", node->assign.varName); 
            printIndent(indent);
            
            printf("%s = ", node->assign.varName);
            generateC_expr(node->assign.value);
            printf(";\n");
            break;

        case NODE_IF:
            printIndent(indent);
            printf("// TAC: tX = condicao; ifFalse tX goto L_ELSE;\n");
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
            
            if (node->control.thenBranch) {
                generateC(node->control.thenBranch, indent + 1);
            }
            
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
            printf("// --- TAC DO LOOP FOR ---\n");
            printIndent(indent);
            printf("// L_START:\n");
            printIndent(indent);
            printf("// t_cond = condicao; ifFalse t_cond goto L_END;\n");
            
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

// Retorna o tamanho exato da string em tempo de compilação
int getStringSize(int index) {
    if (index < varCount) {
        int len = strlen(symbolTable[index].value.s);
        if (len > 0) return len + 1; // Retorna o tamanho da palavra + 1
    }
    return 255; // Padrão de segurança caso seja um read() e a gente não saiba o tamanho
}
