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
void span_display_location(Span *span, char *source_name);
void span_display_in_source(Span *span, char *source);

#endif // SPAN_H
