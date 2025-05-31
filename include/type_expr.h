#ifndef TYPE_EXPR_H
#define TYPE_EXPR_H

#include <stddef.h>

#include "span.h"
#include "interner.h"
#include "bound.h"
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

typedef enum {
    TYPE_EXPR_IDENTIFIER,
    TYPE_EXPR_ARROW,
    TYPE_EXPR_PRODUCT,
}  TypeExprKind;

typedef union {
    TypeIdentifier type_ident;
    TypeArrow type_arrow;
    TypeProduct type_product;
}  TypeExprData;

typedef struct {
    TypeExprKind kind;
    TypeExprData as;
    Span sign_span;
} TypeExpr;

TypeExpr TypeExpr_identifier(InternId identifier_id, Span span);
TypeExpr TypeExpr_arrow(TypeExprIndex from, TypeExprIndex to, Span span);
TypeExpr TypeExpr_product(StringArray names, OffsetArray type_exprs, Span span);
void TypeExpr_display(TypeExpr *type_expr, Arena *arena, Interner *interner, size_t depth);

#endif // TYPE_EXPR_H