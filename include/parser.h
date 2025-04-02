#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "expr.h"

#define CHECK_PARSE_ERROR(result)                         \
    switch ((result).kind) {                              \
        case PARSE_RESULT_ERROR:                          \
            return (result);                              \
        case PARSE_RESULT_LEX_ERROR:                      \
            LexError lex_error = (result).as.lex_error;   \
            return parse_result_new_lex_error(lex_error); \
        case PARSE_RESULT_SUCCESS:                        \
    }                                                     \

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
        UNEXPECTED_EOF
    } kind;
    Token token;
} ParseError;

typedef struct {
    enum {
        PARSE_RESULT_SUCCESS,
        PARSE_RESULT_ERROR,
        PARSE_RESULT_LEX_ERROR,
    } kind;
    union {
        Expr expr;
        Token token;
        ParseError error;
        LexError lex_error;
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
void parse_error_display(ParseError *error, char *source);

ParseResult parse_result_new_success_expr(Expr expr);
ParseResult parse_result_new_success_token(Token token);
ParseResult parse_result_new_error(ParseError error);
ParseResult parse_result_new_lex_error(LexError error);

#endif // PARSER_H