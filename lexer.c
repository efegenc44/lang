#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "token.h"
#include "util.h"

Lexer lexer_new(char *source) {
    return (Lexer) {
        .source = source,
        .cursor = 0
    };
}

Token lexer_next(Lexer *lexer) {
    while (isspace(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    char current_char = lexer_peek(lexer);

    if (isdigit(current_char)) {
        return lexer_integer(lexer);
    }

    if (isalpha(current_char)) {
        return lexer_identifier(lexer);
    }

    TokenKind kind;
    switch (current_char) {
        case '(':
            kind = LEFT_PAREN;
            break;
        case ')':
            kind = RIGHT_PAREN;
            break;
        default:
            unreachable("unknown token");
    }

    return token_new_kind(kind);
}

Token lexer_integer(Lexer *lexer) {
    size_t integer = lexer_advance(lexer) - '0';
    while (isdigit(lexer_peek(lexer))) {
        integer = 10*(integer - '0') + lexer_advance(lexer);
    }

    return token_new_integer(integer);
}

Token lexer_identifier(Lexer *lexer) {
    size_t start = lexer->cursor;
    while (isalnum(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    size_t length = lexer->cursor - start;
    char *lexeme = strndup(&lexer->source[start], length);

    return token_new_identifier(lexeme);
}

char lexer_advance(Lexer *lexer) {
    char current = lexer_peek(lexer);
    lexer->cursor += 1;

    return current;
}

char lexer_peek(Lexer *lexer) {
    return lexer->source[lexer->cursor];
}
