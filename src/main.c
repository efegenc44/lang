#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "token.h"
#include "parser.h"

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
        Parser parser = parser_new(lexer);

        ParseResult result = parser_expr(&parser);
        switch (result.kind) {
            case PARSE_RESULT_ERROR:
                ParseError parse_error = result.as.error;
                parse_error_display(&parse_error, input, "REPL");
                break;
            case PARSE_RESULT_SUCCESS:
                Expr e = result.as.expr;
                expr_display(&e, 0);
                break;
        }
        printf("\n");
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
