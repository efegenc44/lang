#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "expr.h"

typedef struct {
    Lexer lexer;
    LexResult peek;
} Parser;

Parser parser_new(Lexer lexer);
Expr parser_expr(Parser *parser);
Expr parser_binary(Parser *parser, size_t min_prec);
Expr parser_primary(Parser *parser);
Expr parser_finish_paren(Parser *parser);
LexResult parser_advance_token(Parser *parser);
LexResult parser_peek_token(Parser *parser);
Token parser_expect_kind(Parser *parser, TokenKind kind);

#endif // PARSER_H