#include <stdio.h>
#include <stdlib.h>

#include "type.h"

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

bool Type_eq(Type *lhs, Type *rhs) {
    switch (lhs->kind) {
        case TYPE_ISIZE:
            return rhs->kind == TYPE_ISIZE;
        default:
            return false;
    }
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
            printf("{ ");
            for (size_t i = 0; i < type->as.product.names.length; i++) {
                Type *type = &type->as.product.types.types[i];
                InternId intern_id = type->as.product.names.strings[i];
                printf("%s : ", Interner_get(interner, intern_id));
                Type_display(type, arena, interner);
                if (i != type->as.product.names.length) {
                    printf("; ");
                }
            }
            printf(" }");

    }
}
