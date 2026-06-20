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

    // Inicia a compilação!
    AST* programa = parseProgram();

    // NOVO: Verifica se o compilador encontrou algum erro e entrou em Panic Mode
    if (errorCount > 0) {
        printf("\nCOMPILACAO FALHOU: Encontrados %d erro(s) no codigo.\n", errorCount);
    } else {
        // Se estiver tudo bem, avança com a geração normal!
        
        // --- 1. SELEÇÃO DA PRIMEIRA PASSADA (SILENCIOSA) ---
        evaluate(programa);
        int totalTemps = countTemps(programa);

        // Passada 1: Silenciosa para mapear tipos e CONTAR LABELS
        tacPrint = 0;
        tempCount = 0;
        labelCount = 0;
        generateTAC(programa);

        // --- 2. GERAÇÃO DO CÓDIGO C (BASEADO DO TAC) ---
        printf("#include <stdio.h>\n");
        printf("#include <stdbool.h>\n");
        printf("#include <string.h>\n\n"); 
        
        printf("int main() {\n");
        
        printf("    // Variaveis Temporarias do TAC\n");
        for (int i = 0; i < varCount + totalTemps; i++) {
            DataType type = getTempType(i);
            
            // Verifica se a variável atual foi declarada como vetor
            int isArray = isSymbolArray(i);
            int arrSize = getSymbolArraySize(i);
            int arrCols = getSymbolArrayCols(i); 

            if (isArray == 1) {
                // Vetor 1D
                if (type == TYPE_INT) printf("    int t%d[%d];\n", i, arrSize);
                else if (type == TYPE_FLOAT) printf("    float t%d[%d];\n", i, arrSize);
                else if (type == TYPE_CHAR) printf("    char t%d[%d];\n", i, arrSize);
                else if (type == TYPE_BOOL) printf("    bool t%d[%d];\n", i, arrSize);
            } else if (isArray == 2) {
                // Matriz 2D
                if (type == TYPE_INT) printf("    int t%d[%d][%d];\n", i, arrSize, arrCols);
                else if (type == TYPE_FLOAT) printf("    float t%d[%d][%d];\n", i, arrSize, arrCols);
                else if (type == TYPE_CHAR) printf("    char t%d[%d][%d];\n", i, arrSize, arrCols);
                else if (type == TYPE_BOOL) printf("    bool t%d[%d][%d];\n", i, arrSize, arrCols);
            } else {
                // Variável Normal
                if (type == TYPE_FLOAT) printf("    float t%d;\n", i);
                else if (type == TYPE_CHAR) printf("    char t%d;\n", i);
                else if (type == TYPE_STRING) {
                    int size = getStringSize(i);
                    printf("    char t%d[%d];\n", i, size);
                    printf("    int t%d_len;\n", i);
                } else {
                    printf("    int t%d;\n", i);
                }
            }
        }
        
        // Configura para o Modo 2 (Gera o TAC convertido na sintaxe do C)
        tacPrint = 2; 
        tempCount = 0;
        labelCount = 0; 
        
        // Executa a árvore traduzindo os blocos e laços diretamente em rótulos e desvios lineares
        generateTAC(programa); 
        
        printf("    return 0;\n");
        printf("}\n");
    }

    return 0;
}