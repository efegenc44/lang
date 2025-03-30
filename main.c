#include <stdio.h>
#include <stdbool.h>

#include "lexer.h"
#include "token.h"

int main() {
    char *source = "(3 # xyz) * 10";
    Lexer lexer = lexer_new(source);
    for (LexResult result = lexer_next(&lexer); result.kind != DONE; result = lexer_next(&lexer)) {
        if (result.kind == ERROR) {
            lex_error_display(&result.as.error, source);
            break;
        } else {
            token_display(&result.as.token);
            printf("\n");
        }
    }
}
