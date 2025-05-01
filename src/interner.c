#include <stdlib.h>
#include <string.h>

#include "interner.h"

Interner Interner_new() {
    return (Interner) {
        .strings = malloc(INTERNER_DEFAULT_CAPACITY*sizeof(char *)),
        .capacity = INTERNER_DEFAULT_CAPACITY,
        .length = 0
    };
}

void Interner_free(Interner *interner) {
    for (size_t i = 0; i < interner->length; i++) {
        free(Interner_get(interner, i));
    }
    free(interner->strings);
}

InternId Interner_register(Interner *interner, char *string) {
    for (size_t i = 0; i < interner->length; i++) {
        if (strcmp(Interner_get(interner, i), string) == 0) {
            return i;
        }
    }

    if (interner->capacity == interner->length) {
        interner->capacity *= 2;
        interner->strings = realloc(interner->strings, interner->capacity*sizeof(char *));
    }
    interner->strings[interner->length] = string;

    return interner->length++;
}

char *Interner_get(Interner *interner, InternId id) {
    return interner->strings[id];
}

StringArray StringArray_new() {
    return (StringArray) {
        .strings = malloc(STRING_ARRAY_DEFAULT_CAPACITY*sizeof(InternId)),
        .capacity = STRING_ARRAY_DEFAULT_CAPACITY,
        .length = 0
    };
}

void StringArray_free(StringArray *array) {
    free(array->strings);
}

void StringArray_append(StringArray *array, InternId string) {
    if (array->capacity == array->length) {
        array->capacity *= 2;
        array->strings = realloc(array->strings, array->capacity*sizeof(char *));
    }
    array->strings[array->length++] = string;
}

InternId StringArray_pop(StringArray *array) {
    return array->strings[array->length-- - 1];
}
