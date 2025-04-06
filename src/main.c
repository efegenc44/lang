#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "token.h"
#include "parser.h"
#include "interner.h"
#include "resolver.h"

void do_stuff(char *input, char *source_name) {
    Interner interner = Interner_new();
    ExprArray expr_array = ExprArray_new();
    Lexer lexer = Lexer_new(input, &interner);
    Parser parser = Parser_new(lexer, &expr_array);
    Resolver resolver = Resolver_new();

    ParseResult parse_result = Parser_expr(&parser);
    switch (parse_result.kind) {
        case PARSE_RESULT_ERROR:
            ParseError parse_error = parse_result.as.error;
            ParseError_display(&parse_error, &interner, input, source_name);
            goto end;
        case PARSE_RESULT_SUCCESS:
    }
    ExprIndex expr_index = parse_result.as.expr_index;

    ResolveResult resolve_result = Resolver_resolve(&resolver, &expr_array, expr_index);
    switch (resolve_result.kind) {
        case RESOLVE_RESULT_ERROR:
            ResolveError resolve_error = resolve_result.error;
            ResolveError_display(&resolve_error, &interner, input, source_name);
            goto end;
        case RESOLVE_RESULT_SUCCESS:
    }

    Expr expr = ExprArray_get(&expr_array, expr_index);
    Expr_display(&expr, &expr_array, &interner, 0);
    printf("\n");

end:
    Resolver_free(&resolver);
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

        do_stuff(input, "REPL");
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

    do_stuff(content, argv[0]);
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
