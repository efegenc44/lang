#include <stdlib.h>
#include <string.h>

#include "arena.h"

Arena Arena_new() {
    return (Arena) {
        .data = malloc(ARENA_DEFAULT_CAPACITY),
        .capacity = ARENA_DEFAULT_CAPACITY,
        .length = 0
    };
}

void Arena_free(Arena *arena) {
    free(arena->data);
}

Offset _Arena_put(Arena *arena, void *data, size_t size) {
    if (arena->length + size > arena->capacity) {
        arena->capacity *= 2;
        arena->data = realloc(arena->data, arena->capacity);
    }

    Offset offset = arena->length;
    memcpy(arena->data + arena->length, data, size);
    arena->length += size;

    return offset;
}

void *Arena_get_ptr_offset(Arena *arena, Offset offset) {
    return arena->data + offset;
}

OffsetArray OffsetArray_new() {
    return (OffsetArray) {
        .offsets = malloc(OFFSET_ARRAY_DEFAULT_CAPACITY*sizeof(Offset)),
        .capacity = OFFSET_ARRAY_DEFAULT_CAPACITY,
        .length = 0
    };
}

void OffsetArray_free(OffsetArray *array) {
    free(array->offsets);
}

void OffsetArray_append(OffsetArray *array, Offset offset) {
    if (array->length == array->capacity) {
        array->capacity *= 2;
        array->offsets = realloc(array->offsets, array->capacity*sizeof(Offset));
    }

    array->offsets[array->length++] = offset;
}
