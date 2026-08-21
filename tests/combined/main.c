#include "combined.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    Program p;
    program_load(&p,"combined.rwo");
    program_disassemble(&p,stdout);
    RWInstance* exe=(RWInstance*)malloc(sizeof(RWInstance));
    rw_instance_init(exe,&p);
#include "tests.h"
    program_unload(&p);
    rw_instance_unload(exe);
    return EXIT_SUCCESS;
}
