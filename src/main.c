#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "token.h"
#include "parser.h"

void do_stuff(char *input) {
    Interner interner = Interner_new();
    ExprArray expr_array = ExprArray_new();
    Lexer lexer = Lexer_new(input, &interner);
    Parser parser = Parser_new(lexer, &expr_array);

    ParseResult result = Parser_expr(&parser);
    switch (result.kind) {
        case PARSE_RESULT_ERROR:
            ParseError parse_error = result.as.error;
            ParseError_display(&parse_error, &interner, input, "REPL");
            break;
        case PARSE_RESULT_SUCCESS:
            Expr expr = ExprArray_get(&expr_array, result.as.expr_index);
            Expr_display(&expr, &expr_array, &interner, 0);
            break;
    }
    printf("\n");

    ExprArray_free(&expr_array);
    Interner_free(&interner);
}

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

        do_stuff(input);
    }
}

void from_file(int argc, char *argv[]) {
    FILE *file = fopen(argv[0], "r");
    assert(file != NULL);
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char content[length + 1];
    assert(length == fread(content, sizeof(char), length, file));
    content[length] = '\0';

    do_stuff(content);
}

int main(int argc, char *argv[]) {
    switch (argc) {
        case 1:
            repl();
            break;
        default:
            from_file(--argc, ++argv);
    }
}
