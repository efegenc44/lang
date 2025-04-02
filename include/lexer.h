#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#include "token.h"
#include "span.h"

#define LEXER_ITERATE(result, lexer) \
    for (LexResult result = lexer_next(&(lexer)); result.kind != DONE; result = lexer_next(&(lexer)))

typedef struct {
    char *source;
    size_t cursor;
    size_t row;
    size_t column;
} Lexer;

typedef struct {
    enum {
        UNKNOWN_TOKEN_START
    } kind;
    char data;
    Span span;
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
LexResult lexer_next(Lexer *lexer);
LexResult lexer_integer(Lexer *lexer);
LexResult lexer_identifier(Lexer *lexer);
char lexer_advance(Lexer *lexer);
char lexer_peek(Lexer *lexer);
Span lexer_span(Lexer *lexer, size_t start);

LexError lex_error_new_uts(char ch, Span span);
void lex_error_display(LexError *error, char *source);

LexResult lex_result_new_success(Token token);
LexResult lex_result_new_done();
LexResult lex_result_new_error(LexError error);

#endif // LEXER_H
