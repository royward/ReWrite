#include "combined_native.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    (void)argc;
    RWInstance* exe=setup_rw_instance(1000000);
#include "tests.h"
    free_rw_instance(exe);
    return EXIT_SUCCESS;
}
