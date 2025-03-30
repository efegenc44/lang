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
        .cursor = 0,
        .position = {
            .row = 1,
            .column = 1
        }
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

    Position start = lexer_position(lexer);
    TokenKind kind;
    switch (current_char) {
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
            LexError error = lex_error_new_uts(current_char);
            return lex_result_new_error(error);
    }
    lexer_advance(lexer);

    Span span = lexer_span(lexer, start);
    Token token = token_new_kind(kind, span);
    return lex_result_new_success(token);
}

LexResult lexer_integer(Lexer *lexer) {
    Position start = lexer_position(lexer);

    size_t integer = lexer_advance(lexer) - '0';
    while (isdigit(lexer_peek(lexer))) {
        integer = 10*integer + (lexer_advance(lexer) - '0');
    }

    Span span = lexer_span(lexer, start);
    Token token = token_new_integer(integer, span);
    return lex_result_new_success(token);
}

LexResult lexer_identifier(Lexer *lexer) {
    Position start = lexer_position(lexer);
    size_t start_index = lexer->cursor;
    while (isalnum(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    size_t length = lexer->cursor - start_index;
    char *lexeme = strndup(&lexer->source[start_index], length);

    Span span = lexer_span(lexer, start);
    Token token = token_new_identifier(lexeme, span);
    return lex_result_new_success(token);
}

char lexer_advance(Lexer *lexer) {
    char current = lexer_peek(lexer);
    if (current == '\n') {
        lexer->position.row += 1;
        lexer->position.column = 1;
    } else {
        lexer->position.column += 1;
    }
    lexer->cursor += 1;

    return current;
}

char lexer_peek(Lexer *lexer) {
    return lexer->source[lexer->cursor];
}

Position lexer_position(Lexer *lexer) {
    return lexer->position;
}

Span lexer_span(Lexer *lexer, Position start) {
    return (Span) {
        .start = start,
        .end = lexer_position(lexer)
    };
}

LexError lex_error_new_uts(char ch) {
    return (LexError) {
        .kind = UNKNOWN_TOKEN_START,
        .data = ch
    };
}

void lex_error_display(LexError *error) {
    switch (error->kind) {
        case UNKNOWN_TOKEN_START:
            printf("Unknown start of a token : '%c'", error->data);
            break;
        default:
            unreachable("lex_error_display");
    }
}

LexResult lex_result_new_success(Token token) {
    return (LexResult) {
        .kind = SUCCESS,
        .as.token = token
    };
}

LexResult lex_result_new_done() {
    return (LexResult) {
        .kind = DONE,
    };
}

LexResult lex_result_new_error(LexError error) {
    return (LexResult) {
        .kind = ERROR,
        .as.error = error
    };
}
