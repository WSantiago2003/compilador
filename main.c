#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"

extern char input[1000];

int main() {
    printf("===== TESTE TIPAGEM REAL =====\n\n");

    strcpy(input,
        "int i;"
        //"int j;"
        //"float x;"
        //"x = (10 + 5) / (2 + 3);"
        //"bool b;"
        //"x = 10.5;"
        //"x = i + 2.5;"
        //"x = 10.5;"
        "i = 2;"
        //"x = 3.5;"
        //"b = (int)(i * 2 + x) > 7 || (x < 3);"
    );

    printf("Codigo: %s\n", input);
    printf("-----------------------------------\n");

    AST* programa = parseProgram();
    evaluate(programa);

    printSymbolTable();

    return 0;
}