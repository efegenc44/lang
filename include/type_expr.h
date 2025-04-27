#ifndef TYPE_EXPR_H
#define TYPE_EXPR_H

#include <stddef.h>

#include "span.h"
#include "interner.h"
#include "bound.h"

typedef size_t TypeExprIndex;

typedef struct {
    Bound bound;
    InternId identifier_id;
} TypeIdentifier;

typedef struct {
    TypeExprIndex from;
    TypeExprIndex to;
} TypeArrow;

typedef enum {
    TYPE_EXPR_IDENTIFIER,
    TYPE_EXPR_ARROW,
}  TypeExprKind;

typedef union {
    TypeIdentifier type_ident;
    TypeArrow type_arrow;
}  TypeExprData;

typedef struct {
    TypeExprKind kind;
    TypeExprData as;
    Span sign_span;
} TypeExpr;

#define TYPE_EXPR_ARRAY_DEFAULT_CAPACITY 256

typedef struct {
    TypeExpr *type_exprs;
    size_t capacity;
    size_t length;
} TypeExprArray;

TypeExpr TypeExpr_identifier(InternId identifier_id, Span span);
TypeExpr TypeExpr_arrow(TypeExprIndex from, TypeExprIndex to, Span span);
void TypeExpr_display(TypeExpr *type_expr, TypeExprArray *type_expr_array, Interner *interner, size_t depth);

TypeExprArray TypeExprArray_new();
void TypeExprArray_free(TypeExprArray *type_expr_array);
TypeExprIndex TypeExprArray_append(TypeExprArray *type_expr_array, TypeExpr type_expr);
TypeExpr TypeExprArray_get(TypeExprArray *type_expr_array, TypeExprIndex index);

#endif // TYPE_EXPR_H