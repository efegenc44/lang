#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#include "token.h"

typedef struct {
    char *source;
    size_t cursor;
} Lexer;

typedef struct {
    enum {
        SUCCESS,
        DONE,
        ERROR
    } kind;
    Token token;
} LexResult;

Lexer lexer_new(char *source);
void lexer_free(Lexer* lexer);
LexResult lexer_next(Lexer *lexer);
LexResult lexer_integer(Lexer *lexer);
LexResult lexer_identifier(Lexer *lexer);
char lexer_advance(Lexer *lexer);
char lexer_peek(Lexer *lexer);

LexResult lex_result_new_success(Token token);
LexResult lex_result_new_done();
LexResult lex_result_new_error();

#endif // LEXER_H
