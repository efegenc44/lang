#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "token.h"

void repl() {
    while (true) {
        printf(">> ");

        char input[1024];
        assert(fgets(input, 1024, stdin) != NULL);
        if (strncmp("exit", input, 4) == 0) {
            break;
        }

        if (input[0] == '\n') {
            continue;
        }

        Lexer lexer = lexer_new(input);

        printf("TOKENS: \n");
        LEXER_ITERATE(result, lexer) {
            if (result.kind == ERROR) {
                lex_error_display(&result.as.error, input);
                break;
            } else {
                printf("    ");
                token_display(&result.as.token);
                printf("\n");
            }
        }
    }
}

int main(int argc, char *argv[]) {
    switch (argc) {
        case 1:
            repl();
            break;
        default:
            printf("Not implemented");
    }
}
