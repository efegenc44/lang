#ifndef BOUND_H
#define BOUND_H

#include <stddef.h>

typedef size_t BoundId;

typedef enum {
    BOUND_UNDETERMINED,
    BOUND_LOCAL,
    BOUND_GLOBAL
} BoundKind;

typedef struct Bound {
    BoundKind kind;
    BoundId id;
} Bound;

Bound Bound_local(BoundId id);
Bound Bound_global(BoundId id);
Bound Bound_undetermined();
void Bound_display(Bound *bound);

#endif // BOUND_H