#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef struct {
    Lexer lexer;
    LexResult peek;
} Parser;

typedef enum {
    EXPR_INTEGER,
    EXPR_IDENTIFIER,
    EXPR_BINARY
} ExprKind;

typedef struct Expr Expr;

typedef enum {
    BOP_ADD,
    BOP_MUL,
} BOp;

typedef enum {
    ASSOC_RIGHT,
    ASSOC_LEFT,
    ASSOC_NONE
} Assoc;

typedef struct {
    Expr *lhs;
    BOp bop;
    Expr *rhs;
} Binary;

typedef union {
    size_t integer;
    char *identifier;
    Binary binary;
} ExprData;

struct Expr {
    ExprKind kind;
    ExprData as;
    Span sign_span;
};

Parser parser_new(Lexer lexer);
Expr parser_expr(Parser *parser);
Expr parser_binary(Parser *parser, size_t min_prec);
Expr parser_primary(Parser *parser);
LexResult parser_advance_token(Parser *parser);
LexResult parser_peek_token(Parser *parser);
Token parser_expect_kind(Parser *parser, TokenKind kind);

BOp from_token_kind(TokenKind kind);

Expr expr_new_integer(Token token);
Expr expr_new_identifier(Token token);
Expr expr_new_binary(Expr lhs, Token bop, Expr rhs);
void expr_display(Expr *expr, size_t depth);

Expr *box_expr(Expr expr);

#endif // PARSER_H