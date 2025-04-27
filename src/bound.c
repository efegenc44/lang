#include <stdio.h>

#include "bound.h"

Bound Bound_local(BoundId id) {
    return (Bound) {
        .kind = BOUND_LOCAL,
        .id = id
    };
}

Bound Bound_global(BoundId id) {
    return (Bound) {
        .kind = BOUND_GLOBAL,
        .id = id
    };
}

Bound Bound_undetermined() {
    return (Bound) {
        .kind = BOUND_UNDETERMINED
    };
}

void Bound_display(Bound *bound) {
    switch (bound->kind) {
        case BOUND_UNDETERMINED:
            printf("(TBD)");
            break;
        case BOUND_LOCAL:
            printf("(local %ld)", bound->id);
            break;
        case BOUND_GLOBAL:
            // TODO: print the name with Interner
            printf("(global %ld)", bound->id);
            break;
    }
}
