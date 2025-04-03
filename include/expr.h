#ifndef EXPR_H
#define EXPR_H

#include <stddef.h>

#include "token.h"
#include "span.h"

typedef struct Expr Expr;

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
    Expr *lhs;
    BOp bop;
    Expr *rhs;
} Binary;

typedef enum {
    EXPR_INTEGER,
    EXPR_IDENTIFIER,
    EXPR_BINARY
} ExprKind;

typedef union {
    size_t integer;
    char *identifier;
    Binary binary;
} ExprData;

struct Expr {
    ExprKind kind;
    ExprData as;
    Span sign_span;
};

extern const size_t PrecTable[2];
extern const Assoc AssocTable[2];

BOp bop_from_token_kind(TokenKind kind);

Expr Expr_integer(size_t integer, Span span);
Expr Expr_identifier(char *identifier, Span span);
Expr Expr_binary(Expr lhs, BOp bop, Expr rhs, Span span);
void Expr_display(Expr *expr, size_t depth);
Expr *Expr_box(Expr expr);

#endif // EXPR_H