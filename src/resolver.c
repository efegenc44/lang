#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "resolver.h"
#include "bound.h"

LocalStack LocalStack_new() {
    return (LocalStack) {
        .names = malloc(LOCAL_STACK_DEFAULT_CAPACITY*sizeof(InternId)),
        .capacity = LOCAL_STACK_DEFAULT_CAPACITY,
        .length = 0
    };
}

void LocalStack_free(LocalStack *stack) {
    free(stack->names);
}

void LocalStack_push(LocalStack *stack, InternId id) {
    if (stack->capacity == stack->length) {
        stack->capacity *= 2;
        stack->names = realloc(stack->names, stack->capacity*(sizeof(InternId)));
    }
    stack->names[stack->length++] = id;
}

InternId LocalStack_pop(LocalStack *stack) {
    return stack->names[stack->length-- - 1];
}

void LocalStack_truncate(LocalStack *stack, size_t count) {
    stack->length -= count;
}

FindResult LocalStack_find(LocalStack *stack, InternId id) {
    if (stack->length == 0) {
        return (FindResult) {
            .success = false
        };
    }

    for (size_t i = stack->length - 1; i >= 0; i--) {
        if (id == stack->names[i]) {
            return (FindResult) {
                .success = true,
                .id = i
            };
        }
    }

    return (FindResult) {
        .success = false,
    };
}

Resolver Resolver_new() {
    return (Resolver) {
        .locals = LocalStack_new()
    };
}

void Resolver_free(Resolver *resolver) {
    LocalStack_free(&resolver->locals);
}

ResolveResult Resolver_resolve(Resolver *resolver, ExprArray *expr_array, ExprIndex expr_index) {
    Expr *expr = &expr_array->exprs[expr_index];
    switch (expr->kind) {
        case EXPR_INTEGER:
            break;
        case EXPR_IDENTIFIER:
            Identifier *ident = &expr->as.identifier;
            InternId intern_id = ident->identifier_id;
            FindResult result = LocalStack_find(&resolver->locals, intern_id);
            if (result.success) {
                ident->bound = Bound_local(result.id);
            } else {
                ResolveError error = ResolveError_ui(intern_id, expr->sign_span);
                return ResolveResult_error(error);
            }
            break;
        case EXPR_BINARY:
            Binary *binary = &expr->as.binary;
            DOResolve(Resolver_resolve(resolver, expr_array, binary->lhs));
            DOResolve(Resolver_resolve(resolver, expr_array, binary->rhs));
            break;
        case EXPR_LET:
            Let *let = &expr->as.let;
            DOResolve(Resolver_resolve(resolver, expr_array, let->vexpr));
            LocalStack_push(&resolver->locals, let->variable);
                DOResolve(Resolver_resolve(resolver, expr_array, let->rexpr));
            LocalStack_pop(&resolver->locals);
            break;
    };

    return ResolveResult_success();
}

ResolveError ResolveError_ui(InternId identifier, Span span) {
    return (ResolveError) {
        .kind = UNBOUND_IDENTIFIER,
        .identifier = identifier,
        .span = span
    };
}

void ResolveError_display(ResolveError *error, Interner *interner, char *source, char *source_name) {
    Span_display_location(&error->span, source_name);
    printf(": error: ");

    switch (error->kind) {
        case UNBOUND_IDENTIFIER:
            char *identifier = Interner_get(interner, error->identifier);
            printf("Unbound identifier : '%s'", identifier);
            break;
    }

    printf(" (at name resolution)\n");
    Span_display_in_source(&error->span, source);
    printf("\n");
}

ResolveResult ResolveResult_success() {
    return (ResolveResult) {
        .kind = RESOLVE_RESULT_SUCCESS,
    };
}

ResolveResult ResolveResult_error(ResolveError error) {
    return (ResolveResult) {
        .kind = RESOLVE_RESULT_ERROR,
        .error = error,
    };
}
