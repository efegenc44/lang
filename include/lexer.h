#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#include "token.h"
#include "span.h"
#include "interner.h"

#define LEXER_ITERATE(result, lexer) \
    for (LexResult result = Lexer_next(&(lexer)); result.kind != LEX_RESULT_DONE; result = Lexer_next(&(lexer)))

typedef struct {
    char *source;
    size_t cursor;
    size_t row;
    size_t column;
    Interner *interner;
} Lexer;

typedef enum {
    LEX_ERROR_UNKNOWN_TOKEN_START
} LexErrorKind;

typedef struct {
    LexErrorKind kind;
    char data;
    Span span;
} LexError;

typedef enum {
    LEX_RESULT_SUCCESS,
    LEX_RESULT_DONE,
    LEX_RESULT_ERROR
} LexResultKind;

typedef union {
    Token token;
    LexError error;
} LexResultData;

typedef struct {
    LexResultKind kind;
    LexResultData as;
} LexResult;

Lexer Lexer_new(char *source, Interner *interner);
LexResult Lexer_next(Lexer *lexer);
LexResult Lexer_integer(Lexer *lexer);
LexResult Lexer_identifier(Lexer *lexer);
char Lexer_advance(Lexer *lexer);
char Lexer_peek(Lexer *lexer);
Span Lexer_span(Lexer *lexer, size_t start);

LexError LexError_uts(char ch, Span span);
void LexError_display(LexError *error, char *source, char *source_name);

LexResult LexResult_success(Token token);
LexResult LexResult_done();
LexResult LexResult_error(LexError error);

#endif // LEXER_H
