#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

#define Arena_put(arena, data) _Arena_put((arena), &(data), sizeof((data)))
#define Arena_get(type, arena, offset) ((type *) Arena_get_ptr_offset((arena), (offset)))
#define ARENA_DEFAULT_CAPACITY 256

typedef struct {
    void *data;
    size_t capacity;
    size_t length;
} Arena;

typedef size_t Offset;

#define OFFSET_ARRAY_DEFAULT_CAPACITY 256

typedef struct {
    Offset *offsets;
    size_t capacity;
    size_t length;
} OffsetArray;

Arena Arena_new();
void Arena_free(Arena *arena);
Offset _Arena_put(Arena *arena, void *data, size_t size);
void *Arena_get_ptr_offset(Arena *arena, Offset offset);

OffsetArray OffsetArray_new();
void OffsetArray_free(OffsetArray *array);
void OffsetArray_append(OffsetArray *array, Offset offset);

#endif // ARENA_H