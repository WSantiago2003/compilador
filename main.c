#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"

extern char input[1000];

int main() {
    printf("===== TESTE TIPAGEM REAL =====\n\n");

    strcpy(input,
        //"bool b;"
        //"b = ;"
        "int i;"
        "i = 10 / 2;"
        //"a = i + 2.5;"
    );

    printf("Codigo: %s\n", input);
    printf("-----------------------------------\n");

    AST* programa = parseProgram();
    evaluate(programa);

    printSymbolTable();

    return 0;
}