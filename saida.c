#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

int main() {
    // Variaveis Temporarias do TAC
    char* t0 = NULL;
    int t0_len = 0;
    int t0_isDyn = 0;
    char* t1 = NULL;
    int t1_len = 0;
    int t1_isDyn = 0;
    char* t2 = NULL;
    int t2_len = 0;
    int t2_isDyn = 0;
    char* t3 = NULL;
    int t3_len = 0;
    int t3_isDyn = 0;
    if (t0_isDyn == 1 && t0 != NULL) free(t0);
    t0 = "Luana ";
    t0_len = strlen(t0);
    t0_isDyn = 0;
    if (t1_isDyn == 1 && t1 != NULL) free(t1);
    t1 = "Rodrigues";
    t1_len = strlen(t1);
    t1_isDyn = 0;
    t3 = (char*)malloc(strlen(t0) + strlen(t1) + 1);
    strcpy(t3, t0);
    strcat(t3, t1);
    t3_len = strlen(t3);
    t3_isDyn = 1;
    if (t2_isDyn == 1 && t2 != NULL) free(t2);
    t2 = t3;
    t2_len = strlen(t2);
    t2_isDyn = 0;
    printf("%s\n", t2);
    return 0;
}
