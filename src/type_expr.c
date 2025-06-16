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

TypeExpr TypeExpr_product(StringArray names, OffsetArray type_exprs, Span span) {
    return (TypeExpr) {
        .kind = TYPE_EXPR_PRODUCT,
        .as.type_product = {
            .names = names,
            .type_exprs = type_exprs
        },
        .sign_span = span
    };
}

void TypeExpr_display(TypeExpr *type_expr, size_t depth) {
    for (size_t i = 0; i < depth*2; i++) printf(" ");
    switch (type_expr->kind) {
        case TYPE_EXPR_IDENTIFIER:
            TypeIdentifier ident = type_expr->as.type_ident;
            printf("%s ", Interner_get(ident.identifier_id));
            Bound_display(&ident.bound);
            printf("\n");
            break;
        case TYPE_EXPR_ARROW:
            printf("->\n");
            TypeExpr *from = Arena_get(TypeExpr, type_expr->as.type_arrow.from);
            TypeExpr *to = Arena_get(TypeExpr, type_expr->as.type_arrow.to);
            TypeExpr_display(from, depth + 1);
            TypeExpr_display(to, depth + 1);
            break;
        case TYPE_EXPR_PRODUCT:
            TypeProduct *product = &type_expr->as.type_product;
            printf("product\n");
            for (size_t i = 0; i < product->names.length; i++) {
                Offset offset = product->type_exprs.offsets[i];
                InternId intern_id = product->names.strings[i];
                for (size_t i = 0; i < (depth + 1)*2; i++) printf(" ");
                printf("%s :\n", Interner_get(intern_id));
                TypeExpr *texpr = Arena_get(TypeExpr, offset);
                TypeExpr_display(texpr, depth + 2);
            }
            break;
    }
}
