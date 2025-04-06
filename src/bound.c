#include <stdio.h>

#include "bound.h"

Bound Bound_local(BoundId id) {
    return (Bound) {
        .kind = BOUND_LOCAL,
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
    }
}
