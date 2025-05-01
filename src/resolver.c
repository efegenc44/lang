#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "resolver.h"
#include "bound.h"
#include "arena.h"

Resolver Resolver_new() {
    return (Resolver) {
        .locals = StringArray_new(),
        .types = StringArray_new(),
        .defns = StringArray_new(),
    };
}

void Resolver_free(Resolver *resolver) {
    StringArray_new(&resolver->locals);
    StringArray_new(&resolver->types);
    StringArray_new(&resolver->defns);
}

ResolveResult Resolver_collect_names(Resolver *resolver, OffsetArray *decls, Arena *arena) {
    for (size_t i = 0; i < decls->length; i++) {
        Decl *decl = Arena_get(Decl, arena, decls->offsets[i]);
        // TODO: Check for duplicates
        switch (decl->kind) {
            case DECL_BIND:
                Bind *bind = &decl->as.bind;
                StringArray_append(&resolver->defns, bind->name);
                break;
            case DECL_DECL:
                DeclDecl *decldecl = &decl->as.decldecl;
                StringArray_append(&resolver->defns, decldecl->name);
                break;
            case DECL_TYPE:
                Type *type = &decl->as.type;
                StringArray_append(&resolver->types, type->name);
                break;
        }
    }

    return ResolveResult_success();
}

ResolveResult Resolver_decls(Resolver *resolver, OffsetArray *decls, Arena *arena) {
    DOResolve(Resolver_collect_names(resolver, decls, arena));
    for (size_t i = 0; i < decls->length; i++) {
        Decl *decl = Arena_get(Decl, arena, decls->offsets[i]);
        switch (decl->kind) {
            case DECL_BIND:
                Bind *bind = &decl->as.bind;
                DOResolve(Resolver_expr(resolver, decls, arena, bind->expr));
                break;
            case DECL_DECL:
                DeclDecl *decldecl = &decl->as.decldecl;
                DOResolve(Resolver_type_expr(resolver, decls, arena, decldecl->type_expr));
                break;
            case DECL_TYPE:
                break;
        }
    }

    return ResolveResult_success();
}

ResolveResult Resolver_type_expr(Resolver *resolver, OffsetArray *decls, Arena *arena, TypeExprIndex type_expr_index) {
    TypeExpr *type_expr = Arena_get(TypeExpr, arena, type_expr_index);
    switch (type_expr->kind) {
        case TYPE_EXPR_IDENTIFIER:
            TypeIdentifier *ident = &type_expr->as.type_ident;
            InternId intern_id = ident->identifier_id;
            FindResult r = Resolver_local(&resolver->types, intern_id);
            if (r.success) {
                ident->bound = Bound_global(intern_id);
            } else {
                ResolveError error = ResolveError_ui(intern_id, type_expr->sign_span);
                return ResolveResult_error(error);
            }
            break;
        case TYPE_EXPR_ARROW:
            TypeArrow *arrow = &type_expr->as.type_arrow;
            DOResolve(Resolver_type_expr(resolver, decls, arena, arrow->from));
            DOResolve(Resolver_type_expr(resolver, decls, arena, arrow->to));
            break;
    };

    return ResolveResult_success();
}

ResolveResult Resolver_expr(Resolver *resolver, OffsetArray *decls, Arena *arena, ExprIndex expr_index) {
    Expr *expr = Arena_get(Expr, arena, expr_index);
    switch (expr->kind) {
        case EXPR_INTEGER:
            break;
        case EXPR_IDENTIFIER:
            Identifier *ident = &expr->as.identifier;
            InternId intern_id = ident->identifier_id;
            // Look in local scope
            FindResult result = Resolver_local(&resolver->locals, intern_id);
            if (result.success) {
                ident->bound = Bound_local(result.id);
            } else {
                // Look in global scope
                FindResult r = Resolver_local(&resolver->defns, intern_id);
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
            DOResolve(Resolver_expr(resolver, decls, arena, binary->lhs));
            DOResolve(Resolver_expr(resolver, decls, arena, binary->rhs));
            break;
        case EXPR_LET:
            Let *let = &expr->as.let;
            DOResolve(Resolver_expr(resolver, decls, arena, let->vexpr));
            StringArray_append(&resolver->locals, let->variable);
                DOResolve(Resolver_expr(resolver, decls, arena, let->rexpr));
            StringArray_pop(&resolver->locals);
            break;
        case EXPR_LAMBDA:
            Lambda *lambda = &expr->as.lambda;
            StringArray_append(&resolver->locals, lambda->variable);
                DOResolve(Resolver_expr(resolver, decls, arena, lambda->expr));
            StringArray_pop(&resolver->locals);
            break;
        case EXPR_APPLICATION:
            Application *appl = &expr->as.application;
            DOResolve(Resolver_expr(resolver, decls, arena, appl->function));
            DOResolve(Resolver_expr(resolver, decls, arena, appl->argument));
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

FindResult Resolver_local(StringArray *array, InternId id) {
    // de Bruijn indicies
    for (size_t i = 0; i < array->length; i++) {
        if (id == array->strings[array->length - 1 - i]) {
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
