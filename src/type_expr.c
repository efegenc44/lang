#include <assert.h>
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

TypeExpr TypeExpr_lambda(InternId variable, TypeExprIndex expr, Span span) {
    return (TypeExpr) {
        .kind = TYPE_EXPR_LAMBDA,
        .as.type_lambda = {
            .variable = variable,
            .expr = expr
        },
        .sign_span = span
    };
}

TypeExpr TypeExpr_application(TypeExprIndex function, TypeExprIndex argument, Span span) {
    return (TypeExpr) {
        .kind = TYPE_EXPR_APPLICATION,
        .as.type_applicaton = {
            .function = function,
            .argument = argument
        },
        .sign_span = span
    };
}

bool TypeExpr_eq(TypeExpr *lhs, TypeExpr *rhs) {
    switch (lhs->kind) {
        case TYPE_EXPR_IDENTIFIER: {
            if (rhs->kind != TYPE_EXPR_IDENTIFIER) return false;
            TypeIdentifier *lident = &lhs->as.type_ident;
            TypeIdentifier *rident = &rhs->as.type_ident;

            return lident->bound.kind == rident->bound.kind
                && lident->bound.id == rident->bound.id;
        } break;
        case TYPE_EXPR_ARROW: {
            if (rhs->kind != TYPE_EXPR_ARROW) return false;
            TypeArrow *larrow = &lhs->as.type_arrow;
            TypeArrow *rarrow = &rhs->as.type_arrow;

            return TypeExpr_eq(Arena_get(TypeExpr, larrow->from), Arena_get(TypeExpr, rarrow->from))
                && TypeExpr_eq(Arena_get(TypeExpr, larrow->to), Arena_get(TypeExpr, rarrow->to));
        } break;
        case TYPE_EXPR_PRODUCT: {
            if (rhs->kind != TYPE_EXPR_PRODUCT) return false;
            TypeProduct *lproduct = &lhs->as.type_product;
            TypeProduct *rproduct = &rhs->as.type_product;

            if (lproduct->names.length != rproduct->names.length) return false;

            size_t length = lproduct->names.length;
            bool found;
            for (size_t i = 0; i < length; i++) {
                found = false;
                for (size_t j = 0; j < length; j++) {
                    if (lproduct->names.strings[i] == rproduct->names.strings[j]) {
                        found = true;
                        if (!TypeExpr_eq(Arena_get(TypeExpr, lproduct->type_exprs.offsets[i]),
                                         Arena_get(TypeExpr, rproduct->type_exprs.offsets[j]))) {
                            return false;
                        }
                    }
                }
                if (!found) {
                    return false;
                }
            }
            return true;
        } break;
        case TYPE_EXPR_LAMBDA: {
            if (rhs->kind != TYPE_EXPR_LAMBDA) return false;
            TypeLambda *llambda = &lhs->as.type_lambda;
            TypeLambda *rlambda = &rhs->as.type_lambda;

            return TypeExpr_eq(Arena_get(TypeExpr, llambda->expr), Arena_get(TypeExpr, rlambda->expr));
        } break;
        case TYPE_EXPR_APPLICATION: {
            if (rhs->kind != TYPE_EXPR_APPLICATION) return false;
            TypeApplication *lappl = &lhs->as.type_applicaton;
            TypeApplication *rappl = &rhs->as.type_applicaton;

            return TypeExpr_eq(Arena_get(TypeExpr, lappl->function), Arena_get(TypeExpr, rappl->function))
                && TypeExpr_eq(Arena_get(TypeExpr, lappl->argument), Arena_get(TypeExpr, rappl->argument));
        } break;
    }
    assert(0);
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
        case TYPE_EXPR_LAMBDA:
            printf("\\%s", Interner_get(type_expr->as.type_lambda.variable));
            printf(" | ");
            Span_display_start(&type_expr->sign_span);
            printf("\n");
            TypeExpr *bexpr = Arena_get(TypeExpr, type_expr->as.type_lambda.expr);
            TypeExpr_display(bexpr, depth + 1);
            break;
        case TYPE_EXPR_APPLICATION:
            printf("Type Application");
            printf(" | ");
            Span_display_start(&type_expr->sign_span);
            printf("\n");
            TypeExpr *function = Arena_get(TypeExpr, type_expr->as.type_applicaton.function);
            TypeExpr_display(function, depth + 1);
            TypeExpr *argument = Arena_get(TypeExpr, type_expr->as.type_applicaton.argument);
            TypeExpr_display(argument, depth + 1);
            break;
    }
}
