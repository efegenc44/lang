#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "type.h"
#include "arena.h"

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
        array->types = realloc(array->types, array->capacity*sizeof(char *));
    }
    array->types[array->length++] = type;
}

Type TypeArray_pop(TypeArray *array) {
    return array->types[array->length-- - 1];
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

bool Type_eq(Type *lhs, Type *rhs, Arena *arena) {
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
                        if (!Type_eq(&lhs->as.product.types.types[i], &rhs->as.product.types.types[j], arena)) {
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

            Type *lhs_input = Arena_get(Type, arena, lhs->as.function.input);
            Type *rhs_input = Arena_get(Type, arena, rhs->as.function.input);
            if (!Type_eq(lhs_input, rhs_input, arena)) {
                return false;
            }

            Type *lhs_output = Arena_get(Type, arena, lhs->as.function.output);
            Type *rhs_output = Arena_get(Type, arena, rhs->as.function.output);
            if (!Type_eq(lhs_output, rhs_output, arena)) {
                return false;
            }
            return true;
    }
    assert(0);
}

void Type_display(Type *type, Arena *arena, Interner *interner) {
    switch (type->kind) {
        case TYPE_ISIZE:
            printf("isize");
            break;
        case TYPE_ARROW:
            Type *input = Arena_get(Type, arena, type->as.function.input);
            Type *output = Arena_get(Type, arena, type->as.function.output);
            Type_display(input, arena, interner);
            printf(" -> ");
            Type_display(output, arena, interner);
            break;
        case TYPE_PRODUCT:
            printf("{");
            for (size_t i = 0; i < type->as.product.names.length; i++) {
                Type *t = &type->as.product.types.types[i];
                InternId intern_id = type->as.product.names.strings[i];
                printf(" %s : ", Interner_get(interner, intern_id));
                Type_display(t, arena, interner);
                if (i != type->as.product.names.length - 1) {
                    printf(";");
                } else {
                    printf(" ");
                }
            }
            printf("}");
            break;
    }
}
