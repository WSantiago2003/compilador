#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"

int main() {

    strcpy(input,
    "x = 2; \
    if (x > 5) { y = 20; } else { y = 99; }"
    );

    advance();

    parseProgram();

    return 0;
}