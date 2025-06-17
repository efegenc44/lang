#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "expr.h"
#include "type_expr.h"
#include "decl.h"

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

#define BINDParseTE(name, expr)                              \
    ParseResult name##_result = (expr);                      \
    CHECK_PARSE_ERROR(name##_result);                        \
    TypeExprIndex name = (name##_result).as.type_expr_index; \

#define BINDParseT(name, expr)             \
    ParseResult name##_result = (expr);    \
    CHECK_PARSE_ERROR(name##_result);      \
    Token name = (name##_result).as.token; \

#define DOParse(expr)                \
    {                                \
        ParseResult result = (expr); \
        CHECK_PARSE_ERROR(result);   \
    }                                \

typedef struct {
    Lexer lexer;
    LexResult peek;
    OffsetArray decls;
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
    TypeExprIndex type_expr_index;
    Token token;
    OffsetArray decls;
    ParseError error;
} ParseResultData;

typedef struct {
    ParseResultKind kind;
    ParseResultData as;
} ParseResult;

Parser Parser_new(Lexer lexer);
ParseResult Parser_type_expr(Parser *parser);
ParseResult Parser_finish_type_lambda(Parser *parser);
ParseResult Parser_type_arrow(Parser *parser);
ParseResult Parser_type_application(Parser *parser);
ParseResult Parser_type_primary(Parser *parser);
ParseResult Parser_finish_paren_type(Parser *parser);
ParseResult Parser_finish_product_type(Parser *parser, Span span);
ParseResult Parser_decls(Parser *parser);
ParseResult Parser_decl(Parser *parser);
ParseResult Parser_finish_bind(Parser *parser);
ParseResult Parser_finish_decl(Parser *parser);
ParseResult Parser_finish_type(Parser *parser);
ParseResult Parser_expr(Parser *parser);
ParseResult Parser_binary(Parser *parser, size_t min_prec);
ParseResult Parser_application(Parser *parser);
ParseResult Parser_projection(Parser *parser);
ParseResult Parser_primary(Parser *parser);
ParseResult Parser_finish_paren(Parser *parser);
ParseResult Parser_finish_let(Parser *parser);
ParseResult Parser_finish_lambda(Parser *parser);
ParseResult Parser_finish_product(Parser *parser, Span span);
LexResult Parser_advance_token(Parser *parser);
LexResult Parser_peek_token(Parser *parser);
ParseResult Parser_expect_kind(Parser *parser, TokenKind kind);

ParseError ParseError_ut(Token token);
ParseError ParseError_ueof();
ParseError ParseError_lex_error(LexError error);
void ParseError_display(ParseError *error, Interner *interner, char *source, char *source_name);

ParseResult ParseResult_success_type_expr_index(TypeExprIndex type_expr_index);
ParseResult ParseResult_success_expr_index(ExprIndex expr_index);
ParseResult ParseResult_success_decls(OffsetArray array);
ParseResult ParseResult_success_token(Token token);
ParseResult ParseResult_success();
ParseResult ParseResult_error(ParseError error);

#endif // PARSER_H
