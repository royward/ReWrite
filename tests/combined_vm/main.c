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
    codepoint resultc;
    char* resultc1=NULL;
    char* resultc2=NULL;
    int ret_code;

    printf("f:");
    ret_code=rw_f(exe,&result0,&resultc1,&resultc2);
    if(!ret_code && result0==0 && strcmp(resultc1,"abcd")==0 && strcmp(resultc2,"test")==0) { printf("success\n"); } else { printf("fail:%d %ld %s %s\n",ret_code,result0,resultc1,resultc2); };
    free(resultc1);
    free(resultc2);

    printf("g:");
    ret_code=rw_h(exe,"test","hex",0,&resultc1,&resultc);
    if(!ret_code && result0==0 && strcmp(resultc1,"test")==0 && (char)resultc=='e') { printf("success\n"); } else { printf("fail:%d %ld %c %s\n",ret_code,result0,resultc,resultc1); };
    free(resultc1);

    printf("cdr:");
    ret_code=rw_cdr(exe,"test",&resultc1);
    if(!ret_code && strcmp(resultc1,"est")==0) { printf("success\n"); } else { printf("fail:%d %s\n",ret_code,resultc1); };
    free(resultc1);

    printf("len:");
    ret_code=rw_len(exe,"flaccinaucinihilipilification",&result0);
    if(!ret_code && result0==29) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("member:");
    ret_code=rw_member(exe,'h',"flaccinaucinihilipilification",&resultb);
    if(!ret_code && resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,resultb); };

    printf("member:");
    ret_code=rw_member(exe,'x',"flaccinaucinihilipilification",&resultb);
    if(!ret_code && !resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,resultb); };

    printf("listn2:");
    ret_code=rw_listn2(exe,7,&resultc1);
    if(!ret_code && strcmp(resultc1,"abcdefg")==0) { printf("success\n"); } else { printf("fail:%d \"%s\"\n",ret_code,resultc1); };
    free(resultc1);

    printf("listn:");
    ret_code=rw_listn(exe,7,&resultc1);
    if(!ret_code && strcmp(resultc1,"abcdefg")==0) { printf("success\n"); } else { printf("fail:%d \"%s\"\n",ret_code,resultc1); };
    free(resultc1);

    program_unload(&p);
    rw_instance_unload(exe);
    return EXIT_SUCCESS;
}
