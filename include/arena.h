#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

#define Arena_put(data) _Arena_put(&(data), sizeof((data)))
#define Arena_get(type, offset) ((type *) Arena_get_ptr_offset((offset)))
#define ARENA_DEFAULT_CAPACITY 256

typedef struct {
    void *data;
    size_t capacity;
    size_t length;
} Arena;

extern Arena arena;

typedef size_t Offset;

#define OFFSET_ARRAY_DEFAULT_CAPACITY 256

typedef struct {
    Offset *offsets;
    size_t capacity;
    size_t length;
} OffsetArray;

void Arena_init();
void Arena_free();
Offset _Arena_put(void *data, size_t size);
void *Arena_get_ptr_offset(Offset offset);

OffsetArray OffsetArray_new();
void OffsetArray_free(OffsetArray *array);
void OffsetArray_append(OffsetArray *array, Offset offset);

#endif // ARENA_H