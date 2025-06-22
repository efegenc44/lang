#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "type.h"
#include "arena.h"
#include "interner.h"
#include "type_expr.h"

TypeArray TypeArray_new() {
    return (TypeArray) {
        .types = malloc(TYPE_ARRAY_DEFAULT_CAPACITY*sizeof(Type)),
        .capacity = TYPE_ARRAY_DEFAULT_CAPACITY,
        .length = 0
    };
}

void TypeArray_free(TypeArray *array) {
    free(array->types);
}

void TypeArray_append(TypeArray *array, Type type) {
    if (array->capacity == array->length) {
        array->capacity *= 2;
        array->types = realloc(array->types, array->capacity*sizeof(Type));
    }
    array->types[array->length++] = type;
}

Type TypeArray_pop(TypeArray *array) {
    return array->types[array->length-- - 1];
}

TypeArray TypeArray_clone(TypeArray *array) {
    TypeArray cloned = (TypeArray) {
        .types = malloc(array->length * sizeof(Type)),
        .capacity = array->capacity,
        .length = array->length
    };

    cloned.types = memcpy(cloned.types, array->types, array->length * sizeof(Type));
    return cloned;
}

Type Type_isize() {
    return (Type) {
        .kind = TYPE_ISIZE,
    };
}

Type Type_product(StringArray names, TypeArray types) {
    return (Type) {
        .kind = TYPE_PRODUCT,
        .as.product = {
            .names = names,
            .types = types
        }
    };
}

Type Type_arrow(Offset input, Offset output) {
    return (Type) {
        .kind = TYPE_ARROW,
        .as.function = {
            .input = input,
            .output = output
        }
    };
}

Type Type_forall(InternId variable, Offset body_expr, Offset kind, TypeArray closure) {
    return (Type) {
        .kind = TYPE_FORALL,
        .as.forall = {
            .variable = variable,
            .body_expr = body_expr,
            .kind = kind,
            .closure = closure
        }
    };
}

Type Type_kind() {
    return (Type) {
        .kind = TYPE_KIND,
    };
}

Type Type_var(StringArray symbols, Offset kind) {
    return (Type) {
        .kind = TYPE_VAR,
        .as.type_var = {
            .symbols = symbols,
            .kind = kind
        }
    };
}

bool Type_eq(Type *lhs, Type *rhs)
{
    switch (lhs->kind) {
        case TYPE_ISIZE:
            return rhs->kind == TYPE_ISIZE;
        case TYPE_PRODUCT:
            if (rhs->kind != TYPE_PRODUCT) return false;
            if (lhs->as.product.names.length != rhs->as.product.names.length) return false;

            size_t length = lhs->as.product.names.length;
            bool found;
            for (size_t i = 0; i < length; i++) {
                found = false;
                for (size_t j = 0; j < length; j++) {
                    if (lhs->as.product.names.strings[i] == rhs->as.product.names.strings[j]) {
                        found = true;
                        if (!Type_eq(&lhs->as.product.types.types[i], &rhs->as.product.types.types[j])) {
                            return false;
                        }
                    }
                }
                if (!found) {
                    return false;
                }
            }
            return true;
        case TYPE_ARROW:
            if (rhs->kind != TYPE_ARROW) return false;

            Type *lhs_input = Arena_get(Type, lhs->as.function.input);
            Type *rhs_input = Arena_get(Type, rhs->as.function.input);
            if (!Type_eq(lhs_input, rhs_input)) {
                return false;
            }

            Type *lhs_output = Arena_get(Type, lhs->as.function.output);
            Type *rhs_output = Arena_get(Type, rhs->as.function.output);
            if (!Type_eq(lhs_output, rhs_output)) {
                return false;
            }
            return true;
        case TYPE_FORALL: {
            if (rhs->kind != TYPE_FORALL) return false;

            TypeExpr *lhs_expr = Arena_get(TypeExpr, lhs->as.forall.body_expr);
            TypeExpr *rhs_expr = Arena_get(TypeExpr, rhs->as.forall.body_expr);

            return TypeExpr_eq(lhs_expr, rhs_expr);
        }
        case TYPE_KIND: {
            return rhs->kind == TYPE_KIND;
        }
        case TYPE_VAR: {
            if (rhs->kind != TYPE_VAR) return false;
            if (rhs->as.type_var.symbols.length != lhs->as.type_var.symbols.length) return false;

            for (size_t i = 0; i < lhs->as.type_var.symbols.length; i++) {
                if (lhs->as.type_var.symbols.strings[i] != rhs->as.type_var.symbols.strings[i]) {
                    return false;
                }
            }

            return Type_eq(Arena_get(Type, lhs->as.forall.kind), Arena_get(Type, rhs->as.forall.kind));
        }
    }
    assert(0);
}

void Type_display(Type *type) {
    switch (type->kind) {
        case TYPE_ISIZE:
            printf("isize");
            break;
        case TYPE_ARROW:
            Type *input = Arena_get(Type, type->as.function.input);
            Type *output = Arena_get(Type, type->as.function.output);
            Type_display(input);
            printf(" -> ");
            Type_display(output);
            break;
        case TYPE_PRODUCT:
            printf("{");
            for (size_t i = 0; i < type->as.product.names.length; i++) {
                Type *t = &type->as.product.types.types[i];
                InternId intern_id = type->as.product.names.strings[i];
                printf(" %s : ", Interner_get(intern_id));
                Type_display(t);
                if (i != type->as.product.names.length - 1) {
                    printf(";");
                } else {
                    printf(" ");
                }
            }
            printf("}");
            break;
        case TYPE_FORALL:
            printf("forall ");
            printf("%s \n", Interner_get(type->as.forall.variable));
            TypeExpr_display(Arena_get(TypeExpr, type->as.forall.body_expr), 0);
            break;
        case TYPE_KIND:
            printf("*");
            break;
        case TYPE_VAR:
            printf("<>");
            // printf("<%s>", Interner_get(type->as.type_var.var));
            break;
    }
}
