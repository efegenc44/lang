#include <stdio.h>
#include <stdlib.h>

#include "token.h"
#include "span.h"
#include "interner.h"

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

Token Token_identifier(InternId lexeme_id, Span span) {
    return (Token) {
        .kind = IDENTIFIER,
        .as.lexeme_id = lexeme_id,
        .span = span
    };
}

void Token_display(Token *token, Interner *interner) {
    switch (token->kind) {
        case INTEGER:
            printf("%ld", token->as.integer);
            break;
        case IDENTIFIER:
            printf("%s", Interner_get(interner, token->as.lexeme_id));
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
        case EQUALS:
            printf("=");
            break;
        case LET_KEYWORD:
            printf("let");
            break;
        case IN_KEYWORD:
            printf("in");
            break;
    }
}
