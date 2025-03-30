#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "token.h"
#include "util.h"

Lexer lexer_new(char *source) {
    return (Lexer) {
        .source = source,
        .cursor = 0
    };
}

LexResult lexer_next(Lexer *lexer) {
    while (isspace(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    char current_char = lexer_peek(lexer);

    if (current_char == '\0') {
        return lex_result_new_done();
    }

    if (isdigit(current_char)) {
        return lexer_integer(lexer);
    }

    if (isalpha(current_char)) {
        return lexer_identifier(lexer);
    }

    TokenKind kind;
    switch (lexer_advance(lexer)) {
        case '(':
            kind = LEFT_PAREN;
            break;
        case ')':
            kind = RIGHT_PAREN;
            break;
        case '+':
            kind = PLUS;
            break;
        case '*':
            kind = STAR;
            break;
        default:
            return lex_result_new_error();
    }

    Token token = token_new_kind(kind);
    return lex_result_new_success(token);
}

LexResult lexer_integer(Lexer *lexer) {
    size_t integer = lexer_advance(lexer) - '0';
    while (isdigit(lexer_peek(lexer))) {
        integer = 10*integer + (lexer_advance(lexer) - '0');
    }

    Token token = token_new_integer(integer);
    return lex_result_new_success(token);
}

LexResult lexer_identifier(Lexer *lexer) {
    size_t start = lexer->cursor;
    while (isalnum(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    size_t length = lexer->cursor - start;
    char *lexeme = strndup(&lexer->source[start], length);

    Token token = token_new_identifier(lexeme);
    return lex_result_new_success(token);
}

char lexer_advance(Lexer *lexer) {
    char current = lexer_peek(lexer);
    lexer->cursor += 1;

    return current;
}

char lexer_peek(Lexer *lexer) {
    return lexer->source[lexer->cursor];
}

LexResult lex_result_new_success(Token token) {
    return (LexResult) {
        .kind = SUCCESS,
        .token = token
    };
}

LexResult lex_result_new_done() {
    return (LexResult) {
        .kind = DONE,
    };
}

LexResult lex_result_new_error() {
    return (LexResult) {
        .kind = ERROR,
    };
}
