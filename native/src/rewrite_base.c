#include "rewrite_base.h"

RWInstance* setup_rw_instance(size_t sz) {
    RWInstance* ret=(RWInstance*)malloc(sz);
    VMContext* vmc=(VMContext*)ret;
    vmc->jump_buffer=(JumpTarget*)malloc(sizeof(JumpTarget));
    return ret;
}

void free_rw_instance(RWInstance* ret) {
    VMContext* vmc=(VMContext*)ret;
    free(vmc->jump_buffer);
    free(ret);
}

int rw_instance_get_error(RWInstance* exe, uint32_t* line, const char** function) {
    VMContext* vmc=(VMContext*)exe;
    uint32_t err=vmc->error_code;
    if(err!=0) {
        *line=vmc->linenum;
        *function=vmc->function_name;
    }
    return err;
}

void __rw__error(RWInstance* exe, uint32_t code, uint32_t line, uint32_t sym_offset, const char* syms) {
    VMContext* vmc=(VMContext*)exe;
    vmc->error_code=code;
    vmc->linenum=line;
    vmc->function_name=syms+sym_offset;
    longjmp(vmc->jump_buffer->buffer,code);
}
