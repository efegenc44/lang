#include <stdlib.h>

#include "span.h"

Span span_new(Position start, Position end) {
    return (Span) {
        .start = start,
        .end = end
    };
}
