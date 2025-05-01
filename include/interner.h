#ifndef INTERNER_H
#define INTERNER_H

#include <stddef.h>
#include <stdbool.h>

#define INTERNER_DEFAULT_CAPACITY 256
#define STRING_ARRAY_DEFAULT_CAPACITY 256

typedef size_t InternId;

typedef struct {
    char **strings;
    size_t capacity;
    size_t length;
} Interner;

typedef struct {
    InternId *strings;
    size_t capacity;
    size_t length;
} StringArray;

Interner Interner_new();
void Interner_free(Interner *interner);
InternId Interner_register(Interner *interner, char *string);
char *Interner_get(Interner *interner, InternId id);

StringArray StringArray_new();
void StringArray_free(StringArray *array);
void StringArray_append(StringArray *array, InternId string);
InternId StringArray_pop(StringArray *array);

#endif // INTERNER_H