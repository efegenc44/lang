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

void span_display_location(Span *span, char *source_name) {
    printf("%s:", source_name);
    span_display_start(span);
}

void span_display_in_source(Span *span, char *source) {
    size_t cursor = 0;
    for (size_t row = 1; row < span->line; row++) {
        while (source[cursor] != '\n') cursor++;
        cursor++;
    }

    printf(" %5ld | ", span->line);
    for (; source[cursor] != '\n' && source[cursor] != '\0'; cursor++) {
        printf("%c", source[cursor]);
    }
    printf("\n");

    printf("       | ");
    for (size_t i = 1; i < span->start; i++) printf(" ");
    for (size_t i = span->start; i < span->end; i++) printf("^");
}
