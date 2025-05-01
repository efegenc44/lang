#ifndef RESOLVER_H
#define RESOLVER_H

#include <stddef.h>

#include "interner.h"
#include "expr.h"
#include "bound.h"
#include "span.h"
#include "decl.h"

#define CHECK_RESOLVE_ERROR(result)  \
    switch ((result).kind) {         \
        case RESOLVE_RESULT_ERROR:   \
            return (result);         \
        case RESOLVE_RESULT_SUCCESS: \
    }                                \

#define DOResolve(expr)                \
    {                                  \
        ResolveResult result = (expr); \
        CHECK_RESOLVE_ERROR(result);   \
    }                                  \

typedef struct {
    StringArray locals;
    // TODO: Use hash map for declarations
    StringArray types;
    StringArray defns;
} Resolver;

typedef enum {
    UNBOUND_IDENTIFIER,
} ResolveErrorKind;

typedef struct {
    ResolveErrorKind kind;
    InternId identifier;
    Span span;
} ResolveError;

typedef enum {
    RESOLVE_RESULT_SUCCESS,
    RESOLVE_RESULT_ERROR,
} ResolveResultKind;

typedef struct {
    ResolveResultKind kind;
    ResolveError error;
} ResolveResult;

typedef struct {
    bool success;
    BoundId id;
} FindResult;

Resolver Resolver_new();
void Resolver_free(Resolver *resolver);
ResolveResult Resolver_collect_names(Resolver *resolver, DeclArray *decl_array, ExprArray *expr_array, TypeExprArray *type_expr_array);
ResolveResult Resolver_decls(Resolver *resolver, DeclArray *decl_array, ExprArray *expr_array, TypeExprArray *type_expr_array);
ResolveResult Resolver_type_expr(Resolver *resolver, DeclArray *decl_array, TypeExprArray *type_expr_array, TypeExprIndex type_expr_index);
ResolveResult Resolver_expr(Resolver *resolver, DeclArray *decl_array, ExprArray *expr_array, ExprIndex expr_index);

ResolveError ResolveError_ui(InternId identifier, Span span);
void ResolveError_display(ResolveError *error, Interner *interner, char *source, char *source_name);

ResolveResult ResolveResult_success();
ResolveResult ResolveResult_error(ResolveError error);

FindResult Resolver_local(StringArray *array, InternId id);

#endif // RESOLVER_H