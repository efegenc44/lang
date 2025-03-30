#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#include "token.h"

typedef struct {
    char *source;
    size_t cursor;
} Lexer;

Lexer lexer_new(char *source);
void lexer_free(Lexer* lexer);
Token lexer_next(Lexer *lexer);
Token lexer_integer(Lexer *lexer);
Token lexer_identifier(Lexer *lexer);
char lexer_advance(Lexer *lexer);
char lexer_peek(Lexer *lexer);

#endif // LEXER_H
