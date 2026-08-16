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
