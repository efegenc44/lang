#include <stdio.h>
#include <stdlib.h>

#include "token.h"
#include "util.h"

Token token_new_kind(TokenKind kind) {
    return (Token) {
        .kind = kind
    };
}

Token token_new_integer(size_t integer) {
    return (Token) {
        .kind = INTEGER,
        .as.integer = integer
    };
}

Token token_new_identifier(char *lexeme) {
    return (Token) {
        .kind = IDENTIFIER,
        .as.lexeme = lexeme
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
        default:
            unreachable("token_display");
    }
}
