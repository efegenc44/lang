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

Token Token_kind(TokenKind kind, Span span);
Token Token_integer(size_t integer, Span span);
Token Token_identifier(char *lexeme, Span span);
void Token_free(Token *token);
void Token_display(Token *token);

#endif // TOKEN_H
