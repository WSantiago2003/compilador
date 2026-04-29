#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char input[1000];
int pos = 0;

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
    // pular espaços
    while (input[pos] == ' ' || input[pos] == '\n' || input[pos] == '\t' || input[pos] == '\r')
    pos++;

    char c = input[pos];

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
        }
        token.lexeme[i] = '\0';

        // palavras-chave
        if (strcmp(token.lexeme, "if") == 0) token.type = T_IF;
        else if (strcmp(token.lexeme, "while") == 0) token.type = T_WHILE;
        else if (strcmp(token.lexeme, "for") == 0) token.type = T_FOR;
        else if (strcmp(token.lexeme, "int") == 0) token.type = T_INT_TYPE;
        else if (strcmp(token.lexeme, "float") == 0) token.type = T_FLOAT_TYPE;
        else if (strcmp(token.lexeme, "bool") == 0) token.type = T_BOOL_TYPE;
        else if (strcmp(token.lexeme, "char") == 0) token.type = T_CHAR_TYPE;
        else if (strcmp(token.lexeme, "string") == 0) token.type = T_STRING_TYPE;
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
                printf("Erro léxico: número inválido\n");
                exit(1);
            }
            hasDot = 1;
        }

        token.lexeme[i++] = input[pos++];
    }

    token.lexeme[i] = '\0';
    token.type = hasDot ? T_FLOAT : T_INT;

    return token;
    }

    // STRINGS
    if (c == '"') {
        pos++; // pula "

        while (input[pos] != '"' && input[pos] != '\0') {
            token.lexeme[i++] = input[pos++];
        }

        token.lexeme[i] = '\0';

        if (input[pos] == '\0') {
            printf("Erro léxico: string não fechada\n");
            exit(1);
        }
        pos++; // fecha "

        token.type = T_STRING;
        return token;
    }

    // CHAR
    if (c == '\'') {
        pos++;
        token.lexeme[0] = input[pos++];
        token.lexeme[1] = '\0';

        if (input[pos] != '\'') {
            printf("Erro léxico: char inválido\n");
            exit(1);
        }
        pos++; // fecha '

        token.type = T_CHAR;
        return token;
    }

    // OPERADORES
    switch (c) {
        case '+': token.type = T_PLUS; strcpy(token.lexeme, "+"); break;
        case '-': token.type = T_MINUS; strcpy(token.lexeme, "-"); break;
        case '*': token.type = T_MUL; strcpy(token.lexeme, "*"); break;
        case '/': token.type = T_DIV; strcpy(token.lexeme, "/"); break;

        case '>': token.type = T_GT; strcpy(token.lexeme, ">"); break;
        case '<': token.type = T_LT; strcpy(token.lexeme, "<"); break;

        case '=':
            if(input[pos + 1] == '=') {
                token.type = T_EQ;
                strcpy(token.lexeme, "==");
                pos++; // Consome o segundo '='
            } else {
                token.type = T_ASSIGN;
                strcpy(token.lexeme, "=");
            }
            break;

        case '!':
            if (input[pos+1] == '=') {
                pos++;
                token.type = T_NEQ;
            } else {
                token.type = T_NOT;
            }
            break;

        case '&':
            if (input[pos+1] == '&') {
                pos++;
                token.type = T_AND;
            } else {
                printf("Erro léxico: & inválido\n");
                exit(1);
            }
            break;

        case '|':
            if (input[pos+1] == '|') {
                pos++;
                token.type = T_OR;
            } else {
                printf("Erro léxico: | inválido\n");
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
            printf("Erro léxico: %c\n", c);
            exit(1);
    }
    

    pos++;
    return token;
}
