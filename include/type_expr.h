#ifndef TYPE_EXPR_H
#define TYPE_EXPR_H

#include <stddef.h>
#include <stdbool.h>

#include "span.h"
#include "bound.h"
#include "interner.h"
#include "arena.h"

typedef size_t TypeExprIndex;

typedef struct {
    Bound bound;
    InternId identifier_id;
} TypeIdentifier;

typedef struct {
    TypeExprIndex from;
    TypeExprIndex to;
} TypeArrow;

typedef struct {
    StringArray names;
    OffsetArray type_exprs;
} TypeProduct;

typedef struct {
    InternId variable;
    TypeExprIndex kind;
    TypeExprIndex expr;
} TypeLambda;

typedef struct {
    TypeExprIndex function;
    TypeExprIndex argument;
} TypeApplication;

typedef enum {
    TYPE_EXPR_IDENTIFIER,
    TYPE_EXPR_ARROW,
    TYPE_EXPR_PRODUCT,
    TYPE_EXPR_LAMBDA,
    TYPE_EXPR_APPLICATION,
    // Maybe make kinds seperate
    TYPE_EXPR_KIND,
}  TypeExprKind;

typedef union {
    TypeIdentifier type_ident;
    TypeArrow type_arrow;
    TypeProduct type_product;
    TypeLambda type_lambda;
    TypeApplication type_applicaton;
}  TypeExprData;

typedef struct {
    TypeExprKind kind;
    TypeExprData as;
    Span sign_span;
} TypeExpr;

TypeExpr TypeExpr_identifier(InternId identifier_id, Span span);
TypeExpr TypeExpr_arrow(TypeExprIndex from, TypeExprIndex to, Span span);
TypeExpr TypeExpr_product(StringArray names, OffsetArray type_exprs, Span span);
TypeExpr TypeExpr_lambda(InternId variable, TypeExprIndex kind, TypeExprIndex expr, Span span);
TypeExpr TypeExpr_application(TypeExprIndex function, TypeExprIndex argument, Span span);
TypeExpr TypeExpr_kind(Span span);
bool TypeExpr_eq(TypeExpr *lhs, TypeExpr *rhs);
void TypeExpr_display(TypeExpr *type_expr, size_t depth);

#endif // TYPE_EXPR_H