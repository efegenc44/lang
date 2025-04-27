#ifndef DECL_H
#define DECL_H

#include <stddef.h>

#include "expr.h"
#include "interner.h"
#include "span.h"

typedef enum {
    DECL_BIND,
} DeclKind;

typedef struct {
    InternId name;
    ExprIndex expr;
} Bind;

typedef struct {
    Bind bind;
} DeclData;

typedef struct {
    DeclKind kind;
    DeclData as;
    Span sign_span;
} Decl;

// TODO: Use hash map for declarations

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
void Decl_display(Decl *decl, ExprArray *expr_array, Interner *interner);

DeclMap DeclMap_new();
void DeclMap_free(DeclMap *decl_map);
void DeclMap_add(DeclMap *decl_map, InternId key, Decl decl);
DeclGetResult DeclMap_get(DeclMap *decl_map, InternId key);

#endif // DECL_H