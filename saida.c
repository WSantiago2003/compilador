#include <stdio.h>
#include <stdbool.h>
#include <string.h>

int main() {
    // Variaveis Temporarias do TAC
    char t0[16];
    int t0_len;
    int t1;
    char t2[15];
    int t2_len;
    int t3;
    int t4;
    int t5;
    int t6;
    int t7;
    int t8;
    int t9;
    int t10;
    int t11;
    int t12;
    int t13;
    int t14;
    int t15;
    int t16;
    int t17;
    int t18;
    int t19;
    int t20;
    int t21;
    int t22;
    int t23;
    int t24;
    strcpy(t0, "Testando Onda 1");
    t0_len = 15;
    printf("%s\n", t0);
    t1 = 10;
    printf("%d\n", t1);
t5 = 5;
    t6 = t1 + t5;
    t1 = t6;
    printf("%d\n", t1);
t7 = 3;
    t8 = t1 - t7;
    t1 = t8;
    printf("%d\n", t1);
t9 = 2;
    t10 = t1 * t9;
    t1 = t10;
    printf("%d\n", t1);
t11 = 4;
    t12 = t1 / t11;
    t1 = t12;
    printf("%d\n", t1);
t13 = 1;
    t14 = t1 + t13;
    t1 = t14;
    printf("%d\n", t1);
t15 = 1;
    t16 = t1 - t15;
    t1 = t16;
    printf("%d\n", t1);
    strcpy(t2, "Testando o FOR");
    t2_len = 14;
    printf("%s\n", t2);
    t3 = 0;
    L0:
t17 = 10;
    t18 = t3 <= t17;
    if (!t18) goto L2;
    printf("%d\n", t3);
    L1:
t19 = 2;
    t20 = t3 + t19;
    t3 = t20;
    goto L0;
    L2:
    t4 = 3;
    L3:
t21 = 0;
    t22 = t4 > t21;
    if (!t22) goto L5;
    printf("%d\n", t4);
    L4:
t23 = 1;
    t24 = t4 - t23;
    t4 = t24;
    goto L3;
    L5:
    return 0;
}
