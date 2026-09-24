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
    size_t sz;
    int64_t* array;

    printf("f:");
    exe->allocated=0;
    ret_code=rw_f(exe,&result0,&resultc1,&resultc2);
    if(!ret_code && result0==0 && strcmp(resultc1,"abcd")==0 && strcmp(resultc2,"test")==0) { printf("success\n"); } else { printf("fail:%d %ld %s %s\n",ret_code,result0,resultc1,resultc2); };
    free(resultc1);
    free(resultc2);

    printf("h:");
    exe->allocated=0;
    ret_code=rw_h(exe,"test","hex",0,&resultc1,&resultc);
    if(!ret_code && result0==0 && strcmp(resultc1,"test")==0 && (char)resultc=='e') { printf("success\n"); } else { printf("fail:%d %ld %c %s\n",ret_code,result0,resultc,resultc1); };
    free(resultc1);

    printf("cdr:");
    exe->allocated=0;
    ret_code=rw_cdr(exe,(int64_t[]){6,5,5,3,6},5,&array,&sz);
    if(!ret_code && sz==4 && array[0]==5 && array[3]==6) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,sz); };
    free(array);

    printf("car:");
    exe->allocated=0;
    ret_code=rw_car(exe,"test",&resultc);
    if(!ret_code && resultc=='t') { printf("success\n"); } else { printf("fail:%d %c\n",ret_code,resultc); };

    printf("len:");
    exe->allocated=0;
    ret_code=rw_len(exe,"flaccinaucinihilipilification",&result0);
    if(!ret_code && result0==29) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("member:");
    exe->allocated=0;
    ret_code=rw_member(exe,3,(int64_t[]){6,5,5,3,6},5,&resultb);
    if(!ret_code && resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,resultb); };

    printf("member:");
    exe->allocated=0;
    ret_code=rw_member(exe,7,(int64_t[]){6,5,5,3,6},5,&resultb);
    if(!ret_code && !resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,resultb); };

    printf("listn2:");
    exe->allocated=0;
    ret_code=rw_listn2(exe,7,&resultc1);
    if(!ret_code && strcmp(resultc1,"abcdefg")==0) { printf("success\n"); } else { printf("fail:%d \"%s\"\n",ret_code,resultc1); };
    free(resultc1);

    printf("listn:");
    exe->allocated=0;
    ret_code=rw_listn(exe,7,&resultc1);
    if(!ret_code && strcmp(resultc1,"abcdefg")==0) { printf("success\n"); } else { printf("fail:%d \"%s\"\n",ret_code,resultc1); };
    free(resultc1);

    printf("hw:");
    exe->allocated=0;
    ret_code=rw_hw(exe," ",&resultc1,&resultc2);
    if(!ret_code && strcmp(resultc1,"hello world! hello world!")==0 && strcmp(resultc2," ")==0) { printf("success\n"); } else { printf("fail:%d \"%s\"\n",ret_code,resultc1); };
    free(resultc1);
    free(resultc2);

    printf("strdup:");
    exe->allocated=0;
    ret_code=rw_strdup(exe,"test",&resultc1,&resultc2);
    if(!ret_code && strcmp(resultc1,"test")==0 && strcmp(resultc2,"test")==0) { printf("success\n"); } else { printf("fail:%d \"%s\" \"%s\"\n",ret_code,resultc1,resultc2); };
    free(resultc1);
    free(resultc2);

    printf("string_to_int:");
    exe->allocated=0;
    ret_code=rw_string_to_int(exe,"-126",&result0);
    if(!ret_code && result0==-126) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("roman_to_int:");
    exe->allocated=0;
    ret_code=rw_roman_to_int(exe,"mcmxciv",&result0);
    if(!ret_code && result0==1994) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("testprime:");
    exe->allocated=0;
    ret_code=rw_testprime(exe,6,&resultb);
    if(!ret_code && !resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,resultb); };

    printf("testprime:");
    exe->allocated=0;
    ret_code=rw_testprime(exe,11,&resultb);
    if(!ret_code && resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,resultb); };

    printf("nprime:");
    exe->allocated=0;
    ret_code=rw_nprime(exe,1000,&array,&sz);
    if(!ret_code && sz==1000 && array[999]==7919) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,sz); };

    //printf("nprime:");
    //exe->allocated=0;
    //ret_code=rw_nprime(exe,200000,&array,&sz);
    //if(!ret_code && sz==200000 && array[199999]==2750159) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,sz); };
    //free(array);

    printf("Heap size=%d\n",exe->end_of_heap*8);

    program_unload(&p);
    rw_instance_unload(exe);
    return EXIT_SUCCESS;
}
