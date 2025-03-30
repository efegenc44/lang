#include <stdio.h>
#include <stdlib.h>

#include "token.h"
#include "util.h"
#include "span.h"

Token token_new_kind(TokenKind kind, Span span) {
    return (Token) {
        .kind = kind,
        .span = span
    };
}

Token token_new_integer(size_t integer, Span span) {
    return (Token) {
        .kind = INTEGER,
        .as.integer = integer,
        .span = span
    };
}

Token token_new_identifier(char *lexeme, Span span) {
    return (Token) {
        .kind = IDENTIFIER,
        .as.lexeme = lexeme,
        .span = span
    };
}

void token_free(Token *token) {
    switch (token->kind) {
        case IDENTIFIER:
            free(token->as.lexeme);
        case INTEGER:
        case LEFT_PAREN:
        case RIGHT_PAREN:
            break;
        default:
            unreachable("token_free");
    }
}

void token_display(Token *token) {
    switch (token->kind) {
        case INTEGER:
            printf("%ld", token->as.integer);
            break;
        case IDENTIFIER:
            printf("%s", token->as.lexeme);
            break;
        case LEFT_PAREN:
            printf("(");
            break;
        case RIGHT_PAREN:
            printf(")");
            break;
        case PLUS:
            printf("+");
            break;
        case STAR:
            printf("*");
            break;
        default:
            unreachable("token_display");
    }

    printf(" : start(row: %ld, col: %ld) end(row: %ld, col: %ld)",
           token->span.start.row, token->span.start.column,
           token->span.end.row, token->span.end.column);
}
