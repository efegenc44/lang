#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "expr.h"

#define CHECK_LEX_ERROR(result)                                        \
    if ((result).kind == ERROR) {                                      \
        ParseError error = parse_error_new_lex_error(result.as.error); \
        return parse_result_new_error(error);                          \
    }                                                                  \

#define CHECK_LEX_DONE(result)                     \
    if ((result).kind == DONE) {                   \
        ParseError error = parse_error_new_ueof(); \
        return parse_result_new_error(error);      \
    }                                              \

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

#define BINDParse(name, parse_expr)           \
    ParseResult name##_result = (parse_expr); \
    CHECK_PARSE_ERROR(name##_result);         \
    Expr name = (name##_result).as.expr;      \

#define DOParse(expr)                \
    {                                \
        ParseResult result = (expr); \
        CHECK_PARSE_ERROR(result);   \
    }                                \

typedef struct {
    Lexer lexer;
    LexResult peek;
} Parser;

typedef struct {
    enum {
        UNEXPECTED_TOKEN,
        UNEXPECTED_EOF,
        LEX_ERROR,
    } kind;
    union {
        Token token;
        LexError lex_error;
    } as;
} ParseError;

typedef struct {
    enum {
        PARSE_RESULT_SUCCESS,
        PARSE_RESULT_ERROR,
    } kind;
    union {
        Expr expr;
        Token token; // unnecessary?
        ParseError error;
    } as;
} ParseResult;

Parser parser_new(Lexer lexer);
ParseResult parser_expr(Parser *parser);
ParseResult parser_binary(Parser *parser, size_t min_prec);
ParseResult parser_primary(Parser *parser);
ParseResult parser_finish_paren(Parser *parser);
LexResult parser_advance_token(Parser *parser);
LexResult parser_peek_token(Parser *parser);
ParseResult parser_expect_kind(Parser *parser, TokenKind kind);

ParseError parse_error_new_ut(Token token);
ParseError parse_error_new_ueof();
ParseError parse_error_new_lex_error(LexError error);
void parse_error_display(ParseError *error, char *source, char *source_name);

ParseResult parse_result_new_success_expr(Expr expr);
ParseResult parse_result_new_success_token(Token token);
ParseResult parse_result_new_error(ParseError error);

#endif // PARSER_H
