#ifndef TOKEN_H
#define TOKEN_H

#include <stdlib.h>

#include "span.h"
#include "interner.h"

typedef enum {
    TOKEN_INTEGER,
    TOKEN_IDENTIFIER,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_PLUS,
    TOKEN_STAR,
    TOKEN_EQUALS,
    TOKEN_KEYWORD_LET,
    TOKEN_KEYWORD_IN,
} TokenKind;

typedef union {
    size_t integer;
    InternId lexeme_id;
} TokenData;

typedef struct {
    TokenKind kind;
    TokenData as;
    Span span;
} Token;

Token Token_kind(TokenKind kind, Span span);
Token Token_integer(size_t integer, Span span);
Token Token_identifier(InternId lexeme_id, Span span);
void Token_display(Token *token, Interner *interner);

#endif // TOKEN_H
