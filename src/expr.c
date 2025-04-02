#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "expr.h"

const size_t PrecTable[2] = {
    [BOP_ADD] = 1,
    [BOP_MUL] = 2,
};

const Assoc AssocTable[2] = {
    [BOP_ADD] = ASSOC_LEFT,
    [BOP_MUL] = ASSOC_LEFT,
};

BOp bop_from_token_kind(TokenKind kind) {
    switch (kind) {
        case PLUS: return BOP_ADD;
        case STAR: return BOP_MUL;
        default: assert(false);
    }
}

Expr expr_new_integer(size_t integer, Span span) {
    return (Expr) {
        .kind = EXPR_INTEGER,
        .as.integer = integer,
        .sign_span = span
    };
}

Expr expr_new_identifier(char *identifier, Span span) {
    return (Expr) {
        .kind = EXPR_IDENTIFIER,
        .as.identifier = identifier,
        .sign_span = span
    };
}

Expr expr_new_binary(Expr lhs, BOp bop, Expr rhs, Span span) {
    return (Expr) {
        .kind = EXPR_BINARY,
        // TODO: Memory management
        .as.binary = {
            .lhs = expr_box(lhs),
            .bop = bop,
            .rhs = expr_box(rhs)
        },
        .sign_span = span
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

Expr *expr_box(Expr expr) {
    Expr *ptr = malloc(sizeof(Expr));
    *ptr = expr;

    return ptr;
}
