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

    // Cria a Árvore Sintática Abstrata (AST)
    AST* programa = parseProgram();

    // --- 1. GERAÇÃO DO CÓDIGO INTERMEDIÁRIO (TAC) ---
    printf("\n=== CODIGO INTERMEDIARIO (TAC) ===\n");
    evaluate(programa);
    int totalTemps = countTemps(programa);

    tacPrint = 0;
    tempCount = 0;
    generateTAC(programa);

    // Imprime as declarações dos temporários
    for (int i = 0; i < varCount + totalTemps; i++) {
        DataType type = getTempType(i);

        if (type == TYPE_FLOAT)
            printf("float t%d;\n", i);
        else if (type == TYPE_CHAR)
            printf("char t%d;\n", i);
        else
            printf("int t%d;\n", i);
    }
    printf("\n");

    // Imprime as instruções TAC
    tacPrint = 1;
    tempCount = 0;
    generateTAC(programa);


    // --- 2. GERAÇÃO DO CÓDIGO C ---
    printf("\n=== CODIGO C GERADO ===\n");
    printf("#include <stdio.h>\n");
    printf("#include <stdbool.h>\n\n");
    printf("int main() {\n");
    
    // Inicia a geração de código C com nível de indentação 1 (4 espaços)
    generateC(programa, 1);
    
    printf("    return 0;\n");
    printf("}\n");

    return 0;
}