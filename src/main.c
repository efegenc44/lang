#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "token.h"
#include "parser.h"
#include "interner.h"
#include "resolver.h"
#include "type_checker.h"

void do_stuff(char *input, char *source_name) {
    Interner interner = Interner_new();
    Arena arena = Arena_new();
    Lexer lexer = Lexer_new(input, &interner);
    Parser parser = Parser_new(lexer, &arena);
    Resolver resolver = Resolver_new();
    TypeChecker checker = TypeChecker_new();

    ParseResult parse_result = Parser_decls(&parser);
    switch (parse_result.kind) {
        case PARSE_RESULT_ERROR:
            ParseError parse_error = parse_result.as.error;
            ParseError_display(&parse_error, &interner, input, source_name);
            goto parse_error_end;
        case PARSE_RESULT_SUCCESS:
    }
    OffsetArray decls = parse_result.as.decls;

    ResolveResult resolve_result = Resolver_decls(&resolver, &decls, &arena);
    switch (resolve_result.kind) {
        case RESOLVE_RESULT_ERROR:
            ResolveError resolve_error = resolve_result.error;
            ResolveError_display(&resolve_error, &interner, input, source_name);
            goto end;
        case RESOLVE_RESULT_SUCCESS:
    }

    for (size_t i = 0; i < decls.length; i++) {
        Decl *decl = Arena_get(Decl, &arena, decls.offsets[i]);
        Decl_display(decl, &arena, &interner);
        printf("\n");
    }

    TypeCheckResult type_check_result = TypeChecker_decls(&checker, &decls, &arena);
    switch (type_check_result.kind) {
        case TYPE_CHECK_RESULT_ERROR:
            TypeCheckError type_check_error = type_check_result.as.error;
            TypeCheckResult_display(&type_check_error, &arena, &interner, input, source_name);
            goto end;
        case TYPE_CHECK_RESULT_SUCCESS:
    }

end:
    OffsetArray_free(&decls);
parse_error_end:
    TypeChecker_free(&checker);
    Resolver_free(&resolver);
    Arena_free(&arena);
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
