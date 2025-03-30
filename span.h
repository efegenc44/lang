#ifndef SPAN_H
#define SPAN_H

#include <stdint.h>

typedef struct {
    size_t row;
    size_t column;
} Position;

typedef struct {
    Position start;
    Position end;
} Span;

Span span_new(Position start, Position end);

#endif // SPAN_H
