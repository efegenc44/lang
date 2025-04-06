#ifndef RESOLVER_H
#define RESOLVER_H

#include <stddef.h>

#include "interner.h"
#include "expr.h"
#include "bound.h"
#include "span.h"

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

#define LOCAL_STACK_DEFAULT_CAPACITY 256

typedef struct {
    InternId *names;
    size_t capacity;
    size_t length;
} LocalStack;

typedef struct {
    LocalStack locals;
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

LocalStack LocalStack_new();
void LocalStack_free(LocalStack *stack);
void LocalStack_push(LocalStack *stack, InternId id);
InternId LocalStack_pop(LocalStack *stack);
void LocalStack_truncate(LocalStack *stack, size_t count);
FindResult LocalStack_find(LocalStack *stack, InternId id);

Resolver Resolver_new();
void Resolver_free(Resolver *resolver);
ResolveResult Resolver_resolve(Resolver *resolver, ExprArray *expr_array, ExprIndex expr_index);

ResolveError ResolveError_ui(InternId identifier, Span span);
void ResolveError_display(ResolveError *error, Interner *interner, char *source, char *source_name);

ResolveResult ResolveResult_success();
ResolveResult ResolveResult_error(ResolveError error);

#endif // RESOLVER_H