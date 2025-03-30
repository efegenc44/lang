#include <stdlib.h>

#include "span.h"

Span span_new(size_t line, size_t start, size_t end) {
    return (Span) {
        .line = line,
        .start = start,
        .end = end
    };
}
