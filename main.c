#include <stdio.h>

#include "lexer.h"
#include "token.h"

int main() {
    char *source = "(3 + xyz) * 10.5";
    Lexer lexer = lexer_new(source);
    Token token = lexer_next(&lexer);
    token_display(&token);
}
