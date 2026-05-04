#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

extern char input[1000];

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Uso: %s arquivo.txt\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        printf("Erro ao abrir arquivo\n");
        return 1;
    }

    fread(input, 1, sizeof(input), f);
    fclose(f);

    printf("Codigo lido:\n%s\n", input);
    printf("-----------------------------------\n");

    AST* programa = parseProgram();
    generateTAC(programa);

    return 0;
}