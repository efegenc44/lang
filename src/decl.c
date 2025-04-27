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

DeclMap DeclMap_new() {
    return (DeclMap) {
        .pairs = malloc(DECL_MAP_DEFAULT_CAPACITY*sizeof(DeclPair)),
        .capacity = DECL_MAP_DEFAULT_CAPACITY,
        .length = 0
    };
}

void DeclMap_free(DeclMap *decl_map) {
    free(decl_map->pairs);
}

void DeclMap_add(DeclMap *decl_map, InternId key, Decl decl) {
    if (decl_map->length == decl_map->capacity) {
        decl_map->capacity *= 2;
        decl_map->pairs = realloc(decl_map->pairs, decl_map->capacity*sizeof(DeclPair));
    }

    decl_map->pairs[decl_map->length++] = (DeclPair) {
        .key = key,
        .decl = decl
    };
}

DeclGetResult DeclMap_get(DeclMap *decl_map, InternId key) {
    for (size_t i = 0; i < decl_map->length; i++) {
        if (decl_map->pairs[i].key == key) {
            return (DeclGetResult) {
                .success = true,
                .decl = decl_map->pairs[i].decl
            };
        }
    }

    return (DeclGetResult) {
        .success = false
    };
}
