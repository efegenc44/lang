#include <stdio.h>
#include <stdlib.h>

#include "token.h"
#include "span.h"

Token Token_kind(TokenKind kind, Span span) {
    return (Token) {
        .kind = kind,
        .span = span
    };
}

Token Token_integer(size_t integer, Span span) {
    return (Token) {
        .kind = INTEGER,
        .as.integer = integer,
        .span = span
    };
}

Token Token_identifier(char *lexeme, Span span) {
    return (Token) {
        .kind = IDENTIFIER,
        .as.lexeme = lexeme,
        .span = span
    };
}

void Token_free(Token *token) {
    switch (token->kind) {
        case IDENTIFIER:
            free(token->as.lexeme);
        case INTEGER:
        case LEFT_PAREN:
        case RIGHT_PAREN:
        case PLUS:
        case STAR:
            break;
    }
}

void Token_display(Token *token) {
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
    }
}
