#ifndef SPAN_H
#define SPAN_H

#include <stdint.h>

typedef struct {
    size_t line;
    size_t start;
    size_t end;
} Span;

Span Span_new(size_t line, size_t start, size_t end);
void Span_display_start(Span *span);
void Span_display_location(Span *span, char *source_name);
void Span_display_in_source(Span *span, char *source);

#endif // SPAN_H
