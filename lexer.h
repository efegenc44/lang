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
        UNKNOWN_TOKEN_START
    } kind;
    char data;
} LexError;

typedef struct {
    enum {
        SUCCESS,
        DONE,
        ERROR
    } kind;
    union {
        Token token;
        LexError error;
    } as;
} LexResult;

Lexer lexer_new(char *source);
void lexer_free(Lexer* lexer);
LexResult lexer_next(Lexer *lexer);
LexResult lexer_integer(Lexer *lexer);
LexResult lexer_identifier(Lexer *lexer);
char lexer_advance(Lexer *lexer);
char lexer_peek(Lexer *lexer);

LexError lex_error_new_uts(char ch);
void lex_error_display(LexError *error);

LexResult lex_result_new_success(Token token);
LexResult lex_result_new_done();
LexResult lex_result_new_error(LexError error);

#endif // LEXER_H
