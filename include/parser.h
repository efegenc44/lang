#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "expr.h"

#define CHECK_LEX_ERROR(result)                                   \
    if ((result).kind == LEX_RESULT_ERROR) {                      \
        ParseError error = ParseError_lex_error(result.as.error); \
        return ParseResult_error(error);                          \
    }                                                             \

#define CHECK_LEX_DONE(result)                \
    if ((result).kind == LEX_RESULT_DONE) {   \
        ParseError error = ParseError_ueof(); \
        return ParseResult_error(error);      \
    }                                         \

#define BINDLex(name, expr)               \
    LexResult name##_result = (expr);     \
    CHECK_LEX_ERROR(name##_result);       \
    CHECK_LEX_DONE(name##_result);        \
    Token name = (name##_result).as.token \

#define CHECK_PARSE_ERROR(result)  \
    switch ((result).kind) {       \
        case PARSE_RESULT_ERROR:   \
            return (result);       \
        case PARSE_RESULT_SUCCESS: \
    }                              \

#define BINDParse(name, expr)                       \
    ParseResult name##_result = (expr);             \
    CHECK_PARSE_ERROR(name##_result);               \
    ExprIndex name = (name##_result).as.expr_index; \

#define DOParse(expr)                \
    {                                \
        ParseResult result = (expr); \
        CHECK_PARSE_ERROR(result);   \
    }                                \

typedef struct {
    Lexer lexer;
    LexResult peek;
    ExprArray *expr_array;
} Parser;

typedef enum {
    UNEXPECTED_TOKEN,
    UNEXPECTED_EOF,
    LEX_ERROR,
} ParseErrorKind;

typedef union {
    Token token;
    LexError lex_error;
} ParseErrorData;

typedef struct {
    ParseErrorKind kind;
    ParseErrorData as;
} ParseError;

typedef enum {
    PARSE_RESULT_SUCCESS,
    PARSE_RESULT_ERROR,
} ParseResultKind;

typedef union {
    ExprIndex expr_index;
    Token token; // unnecessary?
    ParseError error;
} ParseResultData;

typedef struct {
    ParseResultKind kind;
    ParseResultData as;
} ParseResult;

Parser Parser_new(Lexer lexer, ExprArray *expr_array);
ParseResult Parser_expr(Parser *parser);
ParseResult Parser_binary(Parser *parser, size_t min_prec);
ParseResult Parser_primary(Parser *parser);
ParseResult Parser_finish_paren(Parser *parser);
LexResult Parser_advance_token(Parser *parser);
LexResult Parser_peek_token(Parser *parser);
ParseResult Parser_expect_kind(Parser *parser, TokenKind kind);

ParseError ParseError_ut(Token token);
ParseError ParseError_ueof();
ParseError ParseError_lex_error(LexError error);
void ParseError_display(ParseError *error, Interner *interner, char *source, char *source_name);

ParseResult ParseResult_success_expr_index(ExprIndex expr_index);
ParseResult ParseResult_success_token(Token token);
ParseResult ParseResult_error(ParseError error);

#endif // PARSER_H
