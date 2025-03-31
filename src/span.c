#include <stdlib.h>
#include <stdio.h>

#include "span.h"

Span span_new(size_t line, size_t start, size_t end) {
    return (Span) {
        .line = line,
        .start = start,
        .end = end
    };
}

void span_display_start(Span *span) {
    printf("%ld:%ld", span->line, span->start);
}