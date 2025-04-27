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

    // de Bruijn indicies
    for (size_t i = 0; i < stack->length; i++) {
        if (id == stack->names[stack->length - 1 - i]) {
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
        .locals = LocalStack_new(),
        .types = LocalStack_new(),
        .defns = LocalStack_new(),
    };
}

void Resolver_free(Resolver *resolver) {
    LocalStack_free(&resolver->locals);
    LocalStack_free(&resolver->types);
    LocalStack_free(&resolver->defns);
}

ResolveResult Resolver_collect_names(Resolver *resolver, DeclMap *decl_map, ExprArray *expr_array, TypeExprArray *type_expr_array) {
    for (size_t i = 0; i < decl_map->length; i++) {
        DeclPair *pair = &decl_map->pairs[i];
        // TODO: Check for duplicates
        switch (pair->decl.kind) {
            case DECL_BIND:
                Bind *bind = &pair->decl.as.bind;
                LocalStack_push(&resolver->defns, bind->name);
                break;
            case DECL_DECL:
                DeclDecl *decldecl = &pair->decl.as.decldecl;
                LocalStack_push(&resolver->defns, decldecl->name);
                break;
            case DECL_TYPE:
                Type *type = &pair->decl.as.type;
                LocalStack_push(&resolver->types, type->name);
                break;
        }
    }

    return ResolveResult_success();
}

ResolveResult Resolver_decls(Resolver *resolver, DeclMap *decl_map, ExprArray *expr_array, TypeExprArray *type_expr_array) {
    DOResolve(Resolver_collect_names(resolver, decl_map, expr_array, type_expr_array));
    for (size_t i = 0; i < decl_map->length; i++) {
        DeclPair *pair = &decl_map->pairs[i];
        switch (pair->decl.kind) {
            case DECL_BIND:
                Bind *bind = &pair->decl.as.bind;
                DOResolve(Resolver_expr(resolver, decl_map, expr_array, bind->expr));
                break;
            case DECL_DECL:
                DeclDecl *decldecl = &pair->decl.as.decldecl;
                DOResolve(Resolver_type_expr(resolver, decl_map, type_expr_array, decldecl->type_expr));
                break;
            case DECL_TYPE:
                break;
        }
    }

    return ResolveResult_success();
}

ResolveResult Resolver_type_expr(Resolver *resolver, DeclMap *decl_map, TypeExprArray *type_expr_array, TypeExprIndex type_expr_index) {
    TypeExpr *type_expr = &type_expr_array->type_exprs[type_expr_index];
    switch (type_expr->kind) {
        case TYPE_EXPR_IDENTIFIER:
            TypeIdentifier *ident = &type_expr->as.type_ident;
            InternId intern_id = ident->identifier_id;
            FindResult r = LocalStack_find(&resolver->types, intern_id);
            if (r.success) {
                ident->bound = Bound_global(intern_id);
            } else {
                printf("merhab\n");
                ResolveError error = ResolveError_ui(intern_id, type_expr->sign_span);
                return ResolveResult_error(error);
            }
            break;
        case TYPE_EXPR_ARROW:
            TypeArrow *arrow = &type_expr->as.type_arrow;
            DOResolve(Resolver_type_expr(resolver, decl_map, type_expr_array, arrow->from));
            DOResolve(Resolver_type_expr(resolver, decl_map, type_expr_array, arrow->to));
            break;
    };

    return ResolveResult_success();
}

ResolveResult Resolver_expr(Resolver *resolver, DeclMap *decl_map, ExprArray *expr_array, ExprIndex expr_index) {
    Expr *expr = &expr_array->exprs[expr_index];
    switch (expr->kind) {
        case EXPR_INTEGER:
            break;
        case EXPR_IDENTIFIER:
            Identifier *ident = &expr->as.identifier;
            InternId intern_id = ident->identifier_id;
            // Look in local scope
            FindResult result = LocalStack_find(&resolver->locals, intern_id);
            if (result.success) {
                ident->bound = Bound_local(result.id);
            } else {
                // Look in global scope
                FindResult r = LocalStack_find(&resolver->defns, intern_id);
                if (r.success) {
                    ident->bound = Bound_global(intern_id);
                } else {
                    ResolveError error = ResolveError_ui(intern_id, expr->sign_span);
                    return ResolveResult_error(error);
                }
            }
            break;
        case EXPR_BINARY:
            Binary *binary = &expr->as.binary;
            DOResolve(Resolver_expr(resolver, decl_map, expr_array, binary->lhs));
            DOResolve(Resolver_expr(resolver, decl_map, expr_array, binary->rhs));
            break;
        case EXPR_LET:
            Let *let = &expr->as.let;
            DOResolve(Resolver_expr(resolver, decl_map, expr_array, let->vexpr));
            LocalStack_push(&resolver->locals, let->variable);
                DOResolve(Resolver_expr(resolver, decl_map, expr_array, let->rexpr));
            LocalStack_pop(&resolver->locals);
            break;
        case EXPR_LAMBDA:
            Lambda *lambda = &expr->as.lambda;
            LocalStack_push(&resolver->locals, lambda->variable);
                DOResolve(Resolver_expr(resolver, decl_map, expr_array, lambda->expr));
            LocalStack_pop(&resolver->locals);
            break;
        case EXPR_APPLICATION:
            Application *appl = &expr->as.application;
            DOResolve(Resolver_expr(resolver, decl_map, expr_array, appl->function));
            DOResolve(Resolver_expr(resolver, decl_map, expr_array, appl->argument));
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
