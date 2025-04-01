#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "parser.h"

Parser parser_new(Lexer lexer) {
    LexResult peek = lexer_next(&lexer);

    return (Parser) {
        .lexer = lexer,
        .peek = peek
    };
}

Expr parser_expr(Parser *parser) {
    return parser_binary(parser, 0);
}

Expr parser_binary(Parser *parser, size_t min_prec) {
    Expr lhs = parser_primary(parser);

    while (true) {
        LexResult peek = parser_peek_token(parser);
        // TODO: Error handling
        assert(peek.kind != ERROR);
        if (peek.kind == DONE) return lhs;
        Token token = peek.as.token;

        switch (token.kind) {
            case PLUS:
            case STAR:
                BOp bop = bop_from_token_kind(token.kind);
                if (PrecTable[bop] < min_prec) return lhs;
                parser_advance_token(parser);
                size_t prec = PrecTable[bop] + (AssocTable[bop] != ASSOC_RIGHT);
                Expr rhs = parser_binary(parser, prec);
                lhs = expr_new_binary(lhs, token, rhs);
                if (AssocTable[bop] == ASSOC_NONE) return lhs;
                break;
            default:
                return lhs;
        }
    }

    return lhs;
}

Expr parser_primary(Parser *parser) {
    LexResult result = parser_advance_token(parser);

    // TODO: Error handling
    assert(result.kind == SUCCESS);
    Token token = result.as.token;

    switch (token.kind) {
        case EXPR_INTEGER:
            return expr_new_integer(token);
        case EXPR_IDENTIFIER:
            return expr_new_identifier(token);
        case LEFT_PAREN:
            return parser_finish_paren(parser);
        default:
            // TODO: Error handling
            // Unexpected token
            assert(false);
    }
}

Expr parser_finish_paren(Parser *parser) {
    Expr expr = parser_expr(parser);
    parser_expect_kind(parser, RIGHT_PAREN);

    return expr;
}

LexResult parser_advance_token(Parser *parser) {
    LexResult peek = parser_peek_token(parser);
    parser->peek = lexer_next(&parser->lexer);

    return peek;
}

LexResult parser_peek_token(Parser *parser) {
    return parser->peek;
}

Token parser_expect_kind(Parser *parser, TokenKind kind) {
    LexResult result = parser_advance_token(parser);
    // TODO: Error handling
    assert(result.kind == SUCCESS);
    Token token = result.as.token;

    // TODO: Error handling
    assert(token.kind == kind);

    return token;
}