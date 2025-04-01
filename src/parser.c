#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "parser.h"

const size_t PrecTable[2] = {
    [BOP_ADD] = 1,
    [BOP_MUL] = 2,
};

const Assoc AssocTable[2] = {
    [BOP_ADD] = ASSOC_LEFT,
    [BOP_MUL] = ASSOC_LEFT,
};

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
        if (peek.kind == DONE) goto end;
        Token token = peek.as.token;

        switch (token.kind) {
            case PLUS:
            case STAR:
                BOp bop = from_token_kind(token.kind);
                if (PrecTable[bop] < min_prec) goto end;
                parser_advance_token(parser);
                size_t prec = PrecTable[bop] + (AssocTable[bop] != ASSOC_RIGHT);
                Expr rhs = parser_binary(parser, prec);
                lhs = expr_new_binary(lhs, token, rhs);
                if (AssocTable[bop] == ASSOC_NONE) goto end;
                break;
            default:
                goto end;
        }
    }
    end:

    return lhs;
}

Expr parser_primary(Parser *parser) {
    LexResult result = parser_advance_token(parser);

    // TODO: Error handling
    assert(result.kind == SUCCESS);
    Token token = result.as.token;

    Expr expr = {0};
    switch (token.kind) {
        case EXPR_INTEGER:
            expr = expr_new_integer(token);
            break;
        case EXPR_IDENTIFIER:
            expr = expr_new_identifier(token);
            break;
        case LEFT_PAREN:
            expr = parser_expr(parser);
            parser_expect_kind(parser, RIGHT_PAREN);
            break;
        default:
            // TODO: Error handling
            // Unexpected token
            assert(false);
    }

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

BOp from_token_kind(TokenKind kind) {
    switch (kind) {
        case PLUS: return BOP_ADD;
        case STAR: return BOP_MUL;
        default: assert(false);
    }
}

Expr expr_new_integer(Token token) {
    return (Expr) {
        .kind = EXPR_INTEGER,
        .as.integer = token.as.integer,
        .sign_span = token.span
    };
}

Expr expr_new_identifier(Token token) {
    return (Expr) {
        .kind = EXPR_IDENTIFIER,
        .as.identifier = token.as.lexeme,
        .sign_span = token.span
    };
}

Expr expr_new_binary(Expr lhs, Token bop, Expr rhs) {
    return (Expr) {
        .kind = EXPR_BINARY,
        // TODO: Memory management
        .as.binary = {
            .lhs = box_expr(lhs),
            .bop = from_token_kind(bop.kind),
            .rhs = box_expr(rhs)
        },
        .sign_span = bop.span
    };
}

void expr_display(Expr *expr, size_t depth) {
    for (size_t i = 0; i < depth*2; i++) printf(" ");
    switch (expr->kind) {
        case EXPR_INTEGER:
            printf("%ld", expr->as.integer);
            printf(" | ");
            span_display_start(&expr->sign_span);
            printf("\n");
            break;
        case EXPR_IDENTIFIER:
            printf("%s", expr->as.identifier);
            printf(" | ");
            span_display_start(&expr->sign_span);
            printf("\n");
            break;
        case EXPR_BINARY:
            switch (expr->as.binary.bop) {
                case BOP_ADD:
                    printf("+");
                    break;
                case BOP_MUL:
                    printf("*");
                    break;
            }
            printf(" | ");
            span_display_start(&expr->sign_span);
            printf("\n");
            expr_display(expr->as.binary.lhs, depth + 1);
            expr_display(expr->as.binary.rhs, depth + 1);
            break;
    }
}

Expr *box_expr(Expr expr) {
    Expr *ptr = malloc(sizeof(Expr));
    *ptr = expr;

    return ptr;
}
