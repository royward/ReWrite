#include "combined.h"
#include <stdio.h>

int main(int argc, char** argv) {
    (void)argc;
    Program p;
    program_load(&p,"combined.rwo");
    program_disassemble(&p,stdout);
    RWInstance exe;
    rw_instance_init(&exe,&p);
#include "tests.h"
    program_unload(&p);
    rw_instance_unload(&exe);
    return EXIT_SUCCESS;
}
