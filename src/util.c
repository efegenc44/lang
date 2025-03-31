#include <stdio.h>
#include <stdlib.h>

#include "util.h"

void unreachable(char *message) {
    printf("UNREACHABLE: \n");
    printf("    %s", message);
    exit(1);
}
