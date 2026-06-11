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

    // COMENTADO PARA NÃO SUJAR O ARQUIVO .C FINAL
    // printf("Codigo lido:\n%s\n", input);
    // printf("-----------------------------------\n");

    AST* programa = parseProgram();

    // --- 1. SELEÇÃO DA PRIMEIRA PASSADA (SILENCIOSA) ---
    evaluate(programa);
    int totalTemps = countTemps(programa);

    // Passada 1: Silenciosa para mapear tipos e CONTAR LABELS (guarda o valor final em labelCount)
    tacPrint = 0;
    tempCount = 0;
    labelCount = 0;
    generateTAC(programa);

    // Armazena a quantidade total de labels gerados na passada silenciosa
    //int totalLabels = labelCount;

    // --- 2. GERAÇÃO DO CÓDIGO C (BASEADO DO TAC) ---
    // COMENTADO PARA NÃO SUJAR O ARQUIVO .C FINAL
    // printf("\n=== CODIGO C GERADO ===\n");
    
    printf("#include <stdio.h>\n");
    printf("#include <stdbool.h>\n");
    printf("#include <string.h>\n\n"); 
    printf("int main() {\n");
    
    // Isso aqui é comentário DENTRO do código C gerado, então tá liberado!
    printf("    // Variaveis Temporarias do TAC\n");
    for (int i = 0; i < varCount + totalTemps; i++) {
        DataType type = getTempType(i);
        if (type == TYPE_FLOAT) printf("    float t%d;\n", i);
        else if (type == TYPE_CHAR) printf("    char t%d;\n", i);
        else if (type == TYPE_STRING) printf("    char t%d[255];\n", i);
        else printf("    int t%d;\n", i);
    }

    // Configura para o Modo 2 (Gera o TAC convertido na sintaxe do C)
    tacPrint = 2; 
    tempCount = 0;
    labelCount = 0; // Reseta para começar a imprimir a partir de L0, L1... de forma idêntica
    
    // Executa a árvore traduzindo os blocos e laços diretamente em rótulos e desvios lineares
    generateTAC(programa); 
    
    printf("    return 0;\n");
    printf("}\n");

    return 0;
}