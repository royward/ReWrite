#include "combined.h"
#include <stdio.h>

int main(int argc, char** argv) {
    (void)argc;
    Program p;
    program_load(&p,"combined.rwo");
    program_disassemble(&p,stdout);
    RWInstance exe;
    rw_instance_init(&exe,&p);
    int64_t result0;
    int64_t result1;
    bool resultb;
    int ret_code;

    printf("eval:");
    ret_code=rw_eval(&exe,3,10,6,&result0);
    if(!ret_code && result0==4) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("fact:");
    ret_code=rw_fact(&exe,10,&result0);
    if(!ret_code && result0==3628800) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("equal:");
    ret_code=rw_equal(&exe,6,7,&resultb);
    if(!ret_code && !resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,(int)resultb); };

    printf("equal:");
    ret_code=rw_equal(&exe,7,7,&resultb);
    if(!ret_code && resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,(int)resultb); };

    printf("fib:");
    ret_code=rw_fib(&exe,10,&result0);
    if(!ret_code && result0==89) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("swap:");
    ret_code=rw_swap(&exe,6,7,&result0,&result1);
    if(!ret_code && result0==7 && result1==6) { printf("success\n"); } else { printf("fail:%d %ld %ld\n",ret_code,result0,result1); };

    program_unload(&p);
    rw_instance_unload(&exe);
    return EXIT_SUCCESS;
    return 0;
}
