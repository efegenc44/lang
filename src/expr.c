#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "expr.h"
#include "resolver.h"

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
        case TOKEN_PLUS: return BOP_ADD;
        case TOKEN_STAR: return BOP_MUL;
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
        .as.identifier = {
            .bound = Bound_undetermined(),
            .identifier_id = identifier_id,
        },
        .sign_span = span
    };
}

Expr Expr_binary(ExprIndex lhs, BOp bop, ExprIndex rhs, Span span) {
    return (Expr) {
        .kind = EXPR_BINARY,
        .as.binary = {
            .lhs = lhs,
            .bop = bop,
            .rhs = rhs
        },
        .sign_span = span
    };
}

Expr Expr_let(InternId variable, ExprIndex vexpr, ExprIndex rexpr, Span span) {
    return (Expr) {
        .kind = EXPR_LET,
        .as.let = {
            .variable = variable,
            .vexpr = vexpr,
            .rexpr = rexpr,
        },
        .sign_span = span
    };
}

void Expr_display(Expr *expr, ExprArray *expr_array, Interner *interner, size_t depth) {
    for (size_t i = 0; i < depth*2; i++) printf(" ");
    switch (expr->kind) {
        case EXPR_INTEGER:
            printf("%ld", expr->as.integer);
            printf(" | ");
            Span_display_start(&expr->sign_span);
            printf("\n");
            break;
        case EXPR_IDENTIFIER:
            Identifier ident = expr->as.identifier;
            printf("%s ", Interner_get(interner, ident.identifier_id));
            Bound_display(&ident.bound);
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
            Expr lhs = ExprArray_get(expr_array, expr->as.binary.lhs);
            Expr rhs = ExprArray_get(expr_array, expr->as.binary.rhs);
            Expr_display(&lhs, expr_array, interner, depth + 1);
            Expr_display(&rhs, expr_array, interner, depth + 1);
            break;
        case EXPR_LET:
            printf("let %s", Interner_get(interner, expr->as.let.variable));
            printf(" | ");
            Span_display_start(&expr->sign_span);
            printf("\n");
            Expr vexpr = ExprArray_get(expr_array, expr->as.let.vexpr);
            Expr rexpr = ExprArray_get(expr_array, expr->as.let.rexpr);
            Expr_display(&vexpr, expr_array, interner, depth + 1);
            printf("\n");
            Expr_display(&rexpr, expr_array, interner, depth + 1);
            break;
    }
}

Expr *Expr_box(Expr expr) {
    Expr *ptr = malloc(sizeof(Expr));
    *ptr = expr;

    return ptr;
}

ExprArray ExprArray_new() {
    return (ExprArray) {
        .exprs = malloc(EXPR_ARRAY_DEFAULT_CAPACITY*sizeof(Expr)),
        .capacity = EXPR_ARRAY_DEFAULT_CAPACITY,
        .length = 0
    };
}

void ExprArray_free(ExprArray *expr_array) {
    free(expr_array->exprs);
}

ExprIndex ExprArray_append(ExprArray *expr_array, Expr expr) {
    if (expr_array->capacity == expr_array->length) {
        expr_array->capacity *= 2;
        expr_array->exprs = realloc(expr_array->exprs, expr_array->capacity*sizeof(Expr));
    }
    expr_array->exprs[expr_array->length] = expr;

    return expr_array->length++;
}

Expr ExprArray_get(ExprArray *expr_array, ExprIndex index) {
    return expr_array->exprs[index];
}