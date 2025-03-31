#ifndef TOKEN_H
#define TOKEN_H

#include <stdlib.h>

#include "span.h"

typedef enum {
    INTEGER,
    IDENTIFIER,
    LEFT_PAREN,
    RIGHT_PAREN,
    PLUS,
    STAR
} TokenKind;

typedef union {
    size_t integer;
    char *lexeme;
} TokenData;

typedef struct {
    TokenKind kind;
    TokenData as;
    Span span;
} Token;

Token token_new_kind(TokenKind kind, Span span);
Token token_new_integer(size_t integer, Span span);
Token token_new_identifier(char *lexeme, Span span);
void token_free(Token *token);
void token_display(Token *token);

#endif // TOKEN_H
