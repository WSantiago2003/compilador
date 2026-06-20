
[!] AVISO (Linha 5, Coluna 8): Variavel 'a' lida sem ser inicializada.

[!] AVISO (Linha 2): Variavel 'lixo' declarada, mas nunca utilizada.
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

int main() {
    // Variaveis Temporarias do TAC
    int t0;
    int t1;
    int t2;
    int t3;
    int t4;
    t2 = 10;
    printf("%d\n", t0);
t3 = 1;
    t4 = t2 + t3;
    t2 = t4;
    return 0;
}
