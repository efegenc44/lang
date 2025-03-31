#ifndef SPAN_H
#define SPAN_H

#include <stdint.h>

typedef struct {
    size_t line;
    size_t start;
    size_t end;
} Span;

Span span_new(size_t line, size_t start, size_t end);
void span_display_start(Span *span);

#endif // SPAN_H
