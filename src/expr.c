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

Expr Expr_integer(size_t integer, Span span) {
    return (Expr) {
        .kind = EXPR_INTEGER,
        .as.integer = integer,
        .sign_span = span
    };
}

Expr Expr_identifier(InternId identifier_id, Span span) {
    return (Expr) {
        .kind = EXPR_IDENTIFIER,
        .as.identifier_id = identifier_id,
        .sign_span = span
    };
}

Expr Expr_binary(Expr lhs, BOp bop, Expr rhs, Span span) {
    return (Expr) {
        .kind = EXPR_BINARY,
        // TODO: Memory management
        .as.binary = {
            .lhs = Expr_box(lhs),
            .bop = bop,
            .rhs = Expr_box(rhs)
        },
        .sign_span = span
    };
}

void Expr_display(Expr *expr, Interner *interner, size_t depth) {
    for (size_t i = 0; i < depth*2; i++) printf(" ");
    switch (expr->kind) {
        case EXPR_INTEGER:
            printf("%ld", expr->as.integer);
            printf(" | ");
            Span_display_start(&expr->sign_span);
            printf("\n");
            break;
        case EXPR_IDENTIFIER:
            printf("%s", Interner_get(interner, expr->as.identifier_id));
            printf(" | ");
            Span_display_start(&expr->sign_span);
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
            Span_display_start(&expr->sign_span);
            printf("\n");
            Expr_display(expr->as.binary.lhs, interner, depth + 1);
            Expr_display(expr->as.binary.rhs, interner, depth + 1);
            break;
    }
}

Expr *Expr_box(Expr expr) {
    Expr *ptr = malloc(sizeof(Expr));
    *ptr = expr;

    return ptr;
}
