#include <stdio.h>
#include <stdlib.h>

#include "type_expr.h"
#include "arena.h"

TypeExpr TypeExpr_identifier(InternId identifier_id, Span span) {
    return (TypeExpr) {
        .kind = TYPE_EXPR_IDENTIFIER,
        .as.type_ident = {
            .bound = Bound_undetermined(),
            .identifier_id = identifier_id
        },
        .sign_span = span
    };
}

TypeExpr TypeExpr_arrow(TypeExprIndex from, TypeExprIndex to, Span span) {
    return (TypeExpr) {
        .kind = TYPE_EXPR_ARROW,
        .as.type_arrow = {
            .from = from,
            .to = to
        },
        .sign_span = span
    };
}

void TypeExpr_display(TypeExpr *type_expr, Arena *arena, Interner *interner, size_t depth) {
    for (size_t i = 0; i < depth*2; i++) printf(" ");
    switch (type_expr->kind) {
        case TYPE_EXPR_IDENTIFIER:
            TypeIdentifier ident = type_expr->as.type_ident;
            printf("%s ", Interner_get(interner, ident.identifier_id));
            Bound_display(&ident.bound);
            printf("\n");
            break;
        case TYPE_EXPR_ARROW:
            printf("->\n");
            TypeExpr *from = Arena_get(TypeExpr, arena, type_expr->as.type_arrow.from);
            TypeExpr *to = Arena_get(TypeExpr, arena, type_expr->as.type_arrow.to);
            TypeExpr_display(from, arena, interner, depth + 1);
            TypeExpr_display(to, arena, interner, depth + 1);
            break;
    }
}

TypeExprArray TypeExprArray_new() {
    return (TypeExprArray) {
        .type_exprs = malloc(TYPE_EXPR_ARRAY_DEFAULT_CAPACITY*sizeof(TypeExpr)),
        .capacity = TYPE_EXPR_ARRAY_DEFAULT_CAPACITY,
        .length = 0
    };
}

void TypeExprArray_free(TypeExprArray *type_expr_array) {
    free(type_expr_array->type_exprs);
}

TypeExprIndex TypeExprArray_append(TypeExprArray *expr_array, TypeExpr expr) {
    if (expr_array->capacity == expr_array->length) {
        expr_array->capacity *= 2;
        expr_array->type_exprs = realloc(expr_array->type_exprs, expr_array->capacity*sizeof(TypeExpr));
    }
    expr_array->type_exprs[expr_array->length] = expr;

    return expr_array->length++;
}

TypeExpr TypeExprArray_get(TypeExprArray *expr_array, TypeExprIndex index) {
    return expr_array->type_exprs[index];
}