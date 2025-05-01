#include <stdio.h>

#include "decl.h"
#include "interner.h"

Decl Decl_bind(InternId name, ExprIndex expr, Span span) {
    return (Decl) {
        .kind = DECL_BIND,
        .as.bind = {
            .name = name,
            .expr = expr
        },
        .sign_span = span
    };
}

Decl Decl_decldecl(InternId name, TypeExprIndex type_expr, Span span) {
    return (Decl) {
        .kind = DECL_DECL,
        .as.decldecl = {
            .name = name,
            .type_expr = type_expr
        },
        .sign_span = span
    };
}

Decl Decl_type(InternId name, Span span) {
    return (Decl) {
        .kind = DECL_TYPE,
        .as.type = {
            .name = name
        },
        .sign_span = span
    };
}

void Decl_display(Decl *decl, ExprArray *expr_array, TypeExprArray *type_expr_array, Interner *interner) {
    switch (decl->kind) {
        case DECL_BIND:
            printf("Bind: %s\n", Interner_get(interner, decl->as.bind.name));
            Expr expr = ExprArray_get(expr_array, decl->as.bind.expr);
            Expr_display(&expr, expr_array, interner, 1);
            break;
        case DECL_DECL:
            printf("Decl: %s\n", Interner_get(interner, decl->as.decldecl.name));
            TypeExpr type_expr = TypeExprArray_get(type_expr_array, decl->as.decldecl.type_expr);
            TypeExpr_display(&type_expr, type_expr_array, interner, 1);
            break;
        case DECL_TYPE:
            printf("Type: %s\n", Interner_get(interner, decl->as.type.name));
            break;
    }
}

DeclArray DeclArray_new() {
    return (DeclArray) {
        .decls = malloc(DECL_ARRAY_DEFAULT_CAPACITY*sizeof(Decl)),
        .capacity = DECL_ARRAY_DEFAULT_CAPACITY,
        .length = 0
    };
}

void DeclArray_free(DeclArray *decl_array) {
    free(decl_array->decls);
}

void DeclArray_append(DeclArray *decl_array, Decl decl) {
    if (decl_array->length == decl_array->capacity) {
        decl_array->capacity *= 2;
        decl_array->decls = realloc(decl_array->decls, decl_array->capacity*sizeof(Decl));
    }

    decl_array->decls[decl_array->length++] = decl;
}
