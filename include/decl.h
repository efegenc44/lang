#ifndef DECL_H
#define DECL_H

#include <stddef.h>

#include "expr.h"
#include "interner.h"
#include "span.h"
#include "type_expr.h"
#include "arena.h"

// TODO: Rename decl to stmt (statement) or something

typedef enum {
    DECL_BIND,
    DECL_DECL,
    DECL_TYPE,
} DeclKind;

typedef struct {
    InternId name;
    ExprIndex expr;
} Bind;

typedef struct {
    InternId name;
    TypeExprIndex type_expr;
} DeclDecl;

typedef struct {
    InternId name;
    Offset type_expr;
} Type;

typedef struct {
    Bind bind;
    DeclDecl decldecl;
    Type type;
} DeclData;

typedef struct {
    DeclKind kind;
    DeclData as;
    Span sign_span;
} Decl;

#define DECL_ARRAY_DEFAULT_CAPACITY 256

typedef struct {
    Decl *decls;
    size_t capacity;
    size_t length;
} DeclArray;

Decl Decl_bind(InternId name, ExprIndex expr, Span span);
Decl Decl_decldecl(InternId name, TypeExprIndex type_expr, Span span);
Decl Decl_type(InternId name, Offset type_expr, Span span);
void Decl_display(Decl *decl, Arena *arena, Interner *interner);

DeclArray DeclArray_new();
void DeclArray_free(DeclArray *decl_array);
void DeclArray_append(DeclArray *decl_array, Decl decl);

#endif // DECL_H