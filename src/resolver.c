#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "resolver.h"
#include "bound.h"
#include "arena.h"
#include "interner.h"

Resolver Resolver_new() {
    return (Resolver) {
        .locals = StringArray_new(),
        .type_locals = StringArray_new(),
        .types = StringArray_new(),
        .defns = StringArray_new(),
    };
}

void Resolver_free(Resolver *resolver) {
    StringArray_free(&resolver->locals);
    StringArray_free(&resolver->type_locals);
    StringArray_free(&resolver->types);
    StringArray_free(&resolver->defns);
}

ResolveResult Resolver_collect_names(Resolver *resolver, OffsetArray *decls) {
    for (size_t i = 0; i < decls->length; i++) {
        Decl *decl = Arena_get(Decl, decls->offsets[i]);
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
                DeclType *type = &decl->as.type;
                StringArray_append(&resolver->types, type->name);
                break;
        }
    }

    return ResolveResult_success();
}

ResolveResult Resolver_decls(Resolver *resolver, OffsetArray *decls) {
    DOResolve(Resolver_collect_names(resolver, decls));
    for (size_t i = 0; i < decls->length; i++) {
        Decl *decl = Arena_get(Decl, decls->offsets[i]);
        switch (decl->kind) {
            case DECL_BIND:
                Bind *bind = &decl->as.bind;
                DOResolve(Resolver_expr(resolver, decls, bind->expr));
                break;
            case DECL_DECL:
                DeclDecl *decldecl = &decl->as.decldecl;
                DOResolve(Resolver_type_expr(resolver, decls, decldecl->type_expr));
                break;
            case DECL_TYPE:
                DeclType *type = &decl->as.type;
                DOResolve(Resolver_type_expr(resolver, decls, type->type_expr));
                break;
        }
    }

    return ResolveResult_success();
}

ResolveResult Resolver_type_expr(Resolver *resolver, OffsetArray *decls, TypeExprIndex type_expr_index) {
    TypeExpr *type_expr = Arena_get(TypeExpr, type_expr_index);
    switch (type_expr->kind) {
        case TYPE_EXPR_IDENTIFIER:
            TypeIdentifier *ident = &type_expr->as.type_ident;
            InternId intern_id = ident->identifier_id;
               // Look in local scope
                FindResult result = Resolver_local(&resolver->type_locals, intern_id);
                if (result.success) {
                    ident->bound = Bound_local(result.id);
                } else {
                    // Look in global scope
                    FindResult r = Resolver_local(&resolver->types, intern_id);
                    if (r.success) {
                        ident->bound = Bound_global(intern_id);
                    } else {
                        ResolveError error = ResolveError_ui(intern_id, type_expr->sign_span);
                        return ResolveResult_error(error);
                    }
                }
                break;
        case TYPE_EXPR_ARROW:
            TypeArrow *arrow = &type_expr->as.type_arrow;
            DOResolve(Resolver_type_expr(resolver, decls, arrow->from));
            DOResolve(Resolver_type_expr(resolver, decls, arrow->to));
            break;
        case TYPE_EXPR_PRODUCT:
            TypeProduct *product = &type_expr->as.type_product;
            for (size_t i = 0; i < product->names.length; i++) {
                Offset offset = product->type_exprs.offsets[i];
                DOResolve(Resolver_type_expr(resolver, decls, offset));
            }
            break;
        case TYPE_EXPR_LAMBDA:
            TypeLambda *lambda = &type_expr->as.type_lambda;
            StringArray_append(&resolver->type_locals, lambda->variable);
                DOResolve(Resolver_type_expr(resolver, decls, lambda->expr));
            StringArray_pop(&resolver->type_locals);
            break;
        case TYPE_EXPR_APPLICATION:
            TypeApplication *application = &type_expr->as.type_applicaton;
            DOResolve(Resolver_type_expr(resolver, decls, application->function));
            DOResolve(Resolver_type_expr(resolver, decls, application->argument));
            break;
    };

    return ResolveResult_success();
}

ResolveResult Resolver_expr(Resolver *resolver, OffsetArray *decls, ExprIndex expr_index) {
    Expr *expr = Arena_get(Expr, expr_index);
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
            DOResolve(Resolver_expr(resolver, decls, binary->lhs));
            DOResolve(Resolver_expr(resolver, decls, binary->rhs));
            break;
        case EXPR_LET:
            Let *let = &expr->as.let;
            DOResolve(Resolver_expr(resolver, decls, let->vexpr));
            StringArray_append(&resolver->locals, let->variable);
                DOResolve(Resolver_expr(resolver, decls, let->rexpr));
            StringArray_pop(&resolver->locals);
            break;
        case EXPR_LAMBDA:
            Lambda *lambda = &expr->as.lambda;
            StringArray_append(&resolver->locals, lambda->variable);
                DOResolve(Resolver_expr(resolver, decls, lambda->expr));
            StringArray_pop(&resolver->locals);
            break;
        case EXPR_APPLICATION:
            Application *appl = &expr->as.application;
            DOResolve(Resolver_expr(resolver, decls, appl->function));
            DOResolve(Resolver_expr(resolver, decls, appl->argument));
            break;
        case EXPR_PRODUCT:
            Product *product = &expr->as.product;
            for (size_t i = 0; i < product->names.length; i++) {
                Offset offset = product->exprs.offsets[i];
                DOResolve(Resolver_expr(resolver, decls, offset));
            }
            break;
        case EXPR_PROJECTION:
            DOResolve(Resolver_expr(resolver, decls, expr->as.projection.expr));
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

void ResolveError_display(ResolveError *error, char *source, char *source_name) {
    Span_display_location(&error->span, source_name);
    printf(": error: ");

    switch (error->kind) {
        case UNBOUND_IDENTIFIER:
            char *identifier = Interner_get(error->identifier);
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
