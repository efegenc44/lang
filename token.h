#ifndef TOKEN_H
#define TOKEN_H

#include <stdlib.h>

typedef enum {
    INTEGER,
    IDENTIFIER,
    LEFT_PAREN,
    RIGHT_PAREN,
} TokenKind;

typedef union {
    size_t integer;
    char *lexeme;
} TokenData;

typedef struct {
    TokenKind kind;
    TokenData as;
} Token;

Token token_new_kind(TokenKind kind);
Token token_new_integer(size_t integer);
Token token_new_identifier(char *lexeme);
void token_free(Token *token);
void token_display(Token *token);

#endif // TOKEN_H
