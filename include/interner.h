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

extern Interner interner;

typedef struct {
    InternId *strings;
    size_t capacity;
    size_t length;
} StringArray;

void Interner_init();
void Interner_free();
InternId Interner_register(char *string);
char *Interner_get(InternId id);

StringArray StringArray_new();
void StringArray_free(StringArray *array);
void StringArray_append(StringArray *array, InternId string);
InternId StringArray_pop(StringArray *array);

#endif // INTERNER_H