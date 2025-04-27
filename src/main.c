#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "token.h"
#include "parser.h"
#include "interner.h"
#include "resolver.h"

// TODO: Try to find a solution to passing ..._arrays and interner everywhere
// TODO: Try to find a solution for dynamic array code duplication
// some arrays are internally same with different name, maybe can unify those

void do_stuff(char *input, char *source_name) {
    Interner interner = Interner_new();
    ExprArray expr_array = ExprArray_new();
    TypeExprArray type_expr_array = TypeExprArray_new();
    DeclMap decl_map = DeclMap_new();
    Lexer lexer = Lexer_new(input, &interner);
    Parser parser = Parser_new(lexer, &expr_array, &type_expr_array, &decl_map);
    Resolver resolver = Resolver_new();

    ParseResult parse_result = Parser_decls(&parser);
    switch (parse_result.kind) {
        case PARSE_RESULT_ERROR:
            ParseError parse_error = parse_result.as.error;
            ParseError_display(&parse_error, &interner, input, source_name);
            goto end;
        case PARSE_RESULT_SUCCESS:
    }

    ResolveResult resolve_result = Resolver_decls(&resolver, &decl_map, &expr_array, &type_expr_array);
    switch (resolve_result.kind) {
        case RESOLVE_RESULT_ERROR:
            ResolveError resolve_error = resolve_result.error;
            ResolveError_display(&resolve_error, &interner, input, source_name);
            goto end;
        case RESOLVE_RESULT_SUCCESS:
    }

    for (size_t i = 0; i < decl_map.length; i++) {
        Decl *decl = &decl_map.pairs[i].decl;
        Decl_display(decl, &expr_array, &type_expr_array, &interner);
        printf("\n");
    }

end:
    Resolver_free(&resolver);
    DeclMap_free(&decl_map);
    TypeExprArray_free(&type_expr_array);
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
