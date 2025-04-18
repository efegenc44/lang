#ifndef EXPR_H
#define EXPR_H

#include <stddef.h>

#include "token.h"
#include "span.h"
#include "interner.h"
#include "bound.h"

typedef size_t ExprIndex;

typedef struct {
    Bound bound;
    InternId identifier_id;
} Identifier;

typedef enum {
    BOP_ADD,
    BOP_MUL,
} BOp;

typedef enum {
    ASSOC_RIGHT,
    ASSOC_LEFT,
    ASSOC_NONE
} Assoc;

typedef struct {
    ExprIndex lhs;
    BOp bop;
    ExprIndex rhs;
} Binary;

typedef struct {
    InternId variable;
    ExprIndex vexpr;
    ExprIndex rexpr;
} Let;

typedef enum {
    EXPR_INTEGER,
    EXPR_IDENTIFIER,
    EXPR_BINARY,
    EXPR_LET,
} ExprKind;

typedef union {
    size_t integer;
    Identifier identifier;
    Binary binary;
    Let let;
} ExprData;

typedef struct {
    ExprKind kind;
    ExprData as;
    Span sign_span;
} Expr;

#define EXPR_ARRAY_DEFAULT_CAPACITY 256

typedef struct {
    Expr *exprs;
    size_t capacity;
    size_t length;
} ExprArray;

extern const size_t PrecTable[2];
extern const Assoc AssocTable[2];

BOp bop_from_token_kind(TokenKind kind);

Expr Expr_integer(size_t integer, Span span);
Expr Expr_identifier(InternId identifier_id, Span span);
Expr Expr_binary(ExprIndex lhs, BOp bop, ExprIndex rhs, Span span);
Expr Expr_let(InternId variable, ExprIndex vexpr, ExprIndex rexpr, Span span);
void Expr_display(Expr *expr, ExprArray *expr_array, Interner *interner, size_t depth);
Expr *Expr_box(Expr expr);

ExprArray ExprArray_new();
void ExprArray_free(ExprArray *expr_array);
ExprIndex ExprArray_append(ExprArray *expr_array, Expr expr);
Expr ExprArray_get(ExprArray *expr_array, ExprIndex index);

#endif // EXPR_H