#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char input[1000];
int pos = 0;

// NOVO: Contadores Globais de Rastreamento
int currentLine = 1;
int currentCol = 1;

char* tokenToString(TokenType type) {
    switch(type) {
        case T_ID: return "ID";
        case T_INT: return "INT";
        case T_FLOAT: return "FLOAT";
        case T_INT_TYPE: return "INT_TYPE";
        case T_PLUS: return "PLUS";
        case T_ASSIGN: return "ASSIGN";
        case T_SEMICOLON: return "SEMICOLON";
        case T_FLOAT_TYPE: return "FLOAT_TYPE";
        case T_BOOL_TYPE: return "BOOL_TYPE";
        case T_IF: return "IF";
        case T_WHILE: return "WHILE";
        case T_FOR: return "FOR";
        case T_LBRACE: return "LBRACE";
        case T_RBRACE: return "RBRACE";

        case T_GT: return "GT";
        case T_LT: return "LT";
        case T_GTE: return "GTE"; 
        case T_LTE: return "LTE"; 
        case T_EQ: return "EQ";
        case T_NEQ: return "NEQ";
    
        case T_AND: return "AND";
        case T_OR: return "OR";
        case T_NOT: return "NOT";

        case T_LPAREN: return "LPAREN";
        case T_RPAREN: return "RPAREN";

        case T_COMMA: return "COMMA";
        case T_LBRACKET: return "LBRACKET";
        case T_RBRACKET: return "RBRACKET";

        case T_CHAR_TYPE: return "CHAR_TYPE";
        case T_STRING_TYPE: return "STRING_TYPE";

        case T_EOF: return "EOF";

        case T_BOOL: return "BOOL";
        default: return "OUTRO";
    }
}

// AUXILIARES 

int isLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isDigit(char c) {
    return (c >= '0' && c <= '9');
}

// LEXER 

Token nextToken() 
{
    Token token;
    int i = 0;

    memset(token.lexeme, 0, sizeof(token.lexeme));
    
    // NOVO: Pular espaços e rastrear Linhas/Colunas
    while (input[pos] == ' ' || input[pos] == '\n' || input[pos] == '\t' || input[pos] == '\r') {
        if (input[pos] == '\n') {
            currentLine++;    // Desce a linha
            currentCol = 1;   // Volta pro início
        } else if (input[pos] == '\t') {
            currentCol += 4;  // Tabulação
        } else {
            currentCol++;     // Espaço simples
        }
        pos++;
    }

    char c = input[pos];

    // CARIMBA A POSIÇÃO EXATA ONDE O TOKEN COMEÇA
    token.line = currentLine;
    token.column = currentCol;

    // EOF
    if (input[pos] == '\0') {
        token.type = T_EOF;
        strcpy(token.lexeme, "EOF");
        return token;
    }

    // IDENTIFICADOR ou PALAVRA RESERVADA
    if (isLetter(c)) {
        while (isLetter(input[pos]) || isDigit(input[pos])) {
            token.lexeme[i++] = input[pos++];
            currentCol++; // Acompanha o tamanho da palavra
        }
        token.lexeme[i] = '\0';

        // palavras-chave
        if (strcmp(token.lexeme, "if") == 0) token.type = T_IF;
        else if (strcmp(token.lexeme, "else") == 0) token.type = T_ELSE;
        else if (strcmp(token.lexeme, "while") == 0) token.type = T_WHILE;
        else if (strcmp(token.lexeme, "do") == 0) token.type = T_DO;
        else if (strcmp(token.lexeme, "switch") == 0) token.type = T_SWITCH;
        else if (strcmp(token.lexeme, "case") == 0) token.type = T_CASE;
        else if (strcmp(token.lexeme, "default") == 0) token.type = T_DEFAULT;
        else if (strcmp(token.lexeme, "break") == 0) token.type = T_BREAK;
        else if (strcmp(token.lexeme, "continue") == 0) token.type = T_CONTINUE;
        else if (strcmp(token.lexeme, "for") == 0) token.type = T_FOR;
        else if (strcmp(token.lexeme, "int") == 0) token.type = T_INT_TYPE;
        else if (strcmp(token.lexeme, "float") == 0) token.type = T_FLOAT_TYPE;
        else if (strcmp(token.lexeme, "bool") == 0) token.type = T_BOOL_TYPE;
        else if (strcmp(token.lexeme, "char") == 0) token.type = T_CHAR_TYPE;
        else if (strcmp(token.lexeme, "string") == 0) token.type = T_STRING_TYPE;
        else if (strcmp(token.lexeme, "print") == 0) token.type = T_PRINT;
        else if (strcmp(token.lexeme, "read") == 0) token.type = T_READ;
        else if (strcmp(token.lexeme, "true") == 0 || strcmp(token.lexeme, "false") == 0){
            token.type = T_BOOL;
            return token;
        }
        else
            token.type = T_ID;

        return token;
    }

    // NÚMEROS
    if (isDigit(c)) {
        int hasDot = 0;

        while (isDigit(input[pos]) || input[pos] == '.') {
            if (input[pos] == '.') {
                if (hasDot) {
                    printf("Erro lexico na Linha %d, Coluna %d: numero invalido\n", token.line, token.column);
                    exit(1);
                }
                hasDot = 1;
            }

            token.lexeme[i++] = input[pos++];
            currentCol++; // Acompanha o tamanho do número
        }

        token.lexeme[i] = '\0';
        token.type = hasDot ? T_FLOAT : T_INT;

        return token;
    }

    // STRINGS
    if (c == '"') {
        pos++; currentCol++; // pula " inicial

        while (input[pos] != '"' && input[pos] != '\0') {
            token.lexeme[i++] = input[pos++];
            currentCol++; // Acompanha as letras dentro da string
        }

        token.lexeme[i] = '\0';

        if (input[pos] == '\0') {
            printf("Erro lexico na Linha %d, Coluna %d: string nao fechada\n", token.line, token.column);
            exit(1);
        }
        pos++; currentCol++; // fecha "

        token.type = T_STRING;
        return token;
    }

    // CHAR
    if (c == '\'') {
        pos++; currentCol++;
        token.lexeme[0] = input[pos++];
        currentCol++;
        token.lexeme[1] = '\0';

        if (input[pos] != '\'') {
            printf("Erro lexico na Linha %d, Coluna %d: char invalido\n", token.line, token.column);
            exit(1);
        }
        pos++; currentCol++; // fecha '

        token.type = T_CHAR;
        return token;
    }

    // OPERADORES
    switch (c) {
        case '+': 
            if (input[pos + 1] == '+') {
                token.type = T_INC; strcpy(token.lexeme, "++"); pos++; currentCol++;
            } else if (input[pos + 1] == '=') {
                token.type = T_PLUS_ASSIGN; strcpy(token.lexeme, "+="); pos++; currentCol++;
            } else {
                token.type = T_PLUS; strcpy(token.lexeme, "+"); 
            }
            break;
            
        case '-': 
            if (input[pos + 1] == '-') {
                token.type = T_DEC; strcpy(token.lexeme, "--"); pos++; currentCol++;
            } else if (input[pos + 1] == '=') {
                token.type = T_MINUS_ASSIGN; strcpy(token.lexeme, "-="); pos++; currentCol++;
            } else {
                token.type = T_MINUS; strcpy(token.lexeme, "-"); 
            }
            break;

        case '*':
            if (input[pos + 1] == '=') {
                token.type = T_MUL_ASSIGN; strcpy(token.lexeme, "*="); pos++; currentCol++;
            } else {
                token.type = T_MUL; strcpy(token.lexeme, "*");
            }
            break;

        case '/':
            if (input[pos + 1] == '=') {
                token.type = T_DIV_ASSIGN; strcpy(token.lexeme, "/="); pos++; currentCol++;
            } else {
                token.type = T_DIV; strcpy(token.lexeme, "/");
            }
            break;
        case ':': token.type = T_COLON; strcpy(token.lexeme, ":"); break;
        case '>':
            if (input[pos + 1] == '=') {
                token.type = T_GTE; 
                strcpy(token.lexeme, ">=");
                pos++; currentCol++; 
            } else {
                token.type = T_GT;
                strcpy(token.lexeme, ">");
            }
            break;

        case '<':
            if (input[pos + 1] == '=') {
                token.type = T_LTE; 
                strcpy(token.lexeme, "<=");
                pos++; currentCol++; 
            } else {
                token.type = T_LT;
                strcpy(token.lexeme, "<");
            }
            break;
        case '=':
            if(input[pos + 1] == '=') {
                token.type = T_EQ;
                strcpy(token.lexeme, "==");
                pos++; currentCol++; 
            } else {
                token.type = T_ASSIGN;
                strcpy(token.lexeme, "=");
            }
            break;

        case '!':
            if (input[pos+1] == '=') {
                token.type = T_NEQ;
                strcpy(token.lexeme, "!=");
                pos++; currentCol++;
            } else {
                token.type = T_NOT;
                strcpy(token.lexeme, "!");
            }
            break;

        case '&':
            if (input[pos+1] == '&') {
                token.type = T_AND;
                strcpy(token.lexeme, "&&");
                pos++; currentCol++;
            } else {
                printf("Erro lexico na Linha %d, Coluna %d: '&' invalido\n", token.line, token.column);
                exit(1);
            }
            break;

        case '|':
            if (input[pos+1] == '|') {
                token.type = T_OR;
                strcpy(token.lexeme, "||");
                pos++; currentCol++;
            } else {
                printf("Erro lexico na Linha %d, Coluna %d: '|' invalido\n", token.line, token.column);
                exit(1);
            }
            break;

        case '(': token.type = T_LPAREN; strcpy(token.lexeme, "("); break;
        case ')': token.type = T_RPAREN; strcpy(token.lexeme, ")"); break;
        case ';': token.type = T_SEMICOLON; strcpy(token.lexeme, ";"); break;
        case '{': token.type = T_LBRACE; strcpy(token.lexeme, "{"); break;
        case '}': token.type = T_RBRACE; strcpy(token.lexeme, "}"); break;
        case ',': token.type = T_COMMA; strcpy(token.lexeme, ","); break;
        case '[': token.type = T_LBRACKET; strcpy(token.lexeme, "["); break;
        case ']': token.type = T_RBRACKET; strcpy(token.lexeme, "]"); break;

        default:
            printf("Erro lexico na Linha %d, Coluna %d: caractere '%c' nao reconhecido\n", token.line, token.column, c);
            exit(1);
    }
    
    pos++;
    currentCol++; // Soma a coluna para o caractere do switch
    return token;
}