#include "combined_vm.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    Program p;
    program_load(&p,"combined_vm.rwo");
    program_disassemble(&p,stdout);
    RWInstance* exe=(RWInstance*)malloc(sizeof(RWInstance));
    rw_instance_init(exe,&p);

    int64_t result0;
    int64_t result1;
    bool resultb;
    char* resultc;
    int ret_code;

    printf("f:");
    ret_code=rw_f(exe,&result0,&resultc);
    if(!ret_code && result0==0) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    program_unload(&p);
    rw_instance_unload(exe);
    return EXIT_SUCCESS;
}
