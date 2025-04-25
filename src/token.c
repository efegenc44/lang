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
        .kind = TOKEN_INTEGER,
        .as.integer = integer,
        .span = span
    };
}

Token Token_identifier(InternId lexeme_id, Span span) {
    return (Token) {
        .kind = TOKEN_IDENTIFIER,
        .as.lexeme_id = lexeme_id,
        .span = span
    };
}

void Token_display(Token *token, Interner *interner) {
    switch (token->kind) {
        case TOKEN_INTEGER:
            printf("%ld", token->as.integer);
            break;
        case TOKEN_IDENTIFIER:
            printf("%s", Interner_get(interner, token->as.lexeme_id));
            break;
        case TOKEN_LEFT_PAREN:
            printf("(");
            break;
        case TOKEN_RIGHT_PAREN:
            printf(")");
            break;
        case TOKEN_PLUS:
            printf("+");
            break;
        case TOKEN_STAR:
            printf("*");
            break;
        case TOKEN_EQUALS:
            printf("=");
            break;
        case TOKEN_KEYWORD_LET:
            printf("let");
            break;
        case TOKEN_KEYWORD_IN:
            printf("in");
            break;
    }
}
