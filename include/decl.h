#ifndef DECL_H
#define DECL_H

#include <stddef.h>

#include "expr.h"
#include "interner.h"
#include "span.h"
#include "type_expr.h"

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

// TODO: Use hash map for declarations
// TODO: Probably do not need anymore
// we collect names seperately at name resolving
// so a simple array would be enough

#define DECL_MAP_DEFAULT_CAPACITY 256

typedef struct {
    InternId key;
    Decl decl;
} DeclPair;

typedef struct {
    DeclPair *pairs;
    size_t capacity;
    size_t length;
} DeclMap;

typedef struct {
    bool success;
    Decl decl;
} DeclGetResult;

Decl Decl_bind(InternId name, ExprIndex expr, Span span);
Decl Decl_decldecl(InternId name, TypeExprIndex type_expr, Span span);
Decl Decl_type(InternId name, Span span);
void Decl_display(Decl *decl, ExprArray *expr_array, TypeExprArray *type_expr_array, Interner *interner);

DeclMap DeclMap_new();
void DeclMap_free(DeclMap *decl_map);
void DeclMap_add(DeclMap *decl_map, InternId key, Decl decl);
DeclGetResult DeclMap_get(DeclMap *decl_map, InternId key);

#endif // DECL_H