#ifndef REWRITE_BASE_H
#define REWRITE_BASE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <setjmp.h>

typedef struct {
    jmp_buf buffer;
} JumpTarget;

typedef void RWInstance;

typedef struct {
    uint64_t dummy;
    uint32_t instance_size;
    uint32_t instance_max_size;
    uint32_t error_code;
    uint32_t linenum;
    const char* function_name;
    JumpTarget* jump_buffer;
} VMContext;

RWInstance* setup_rw_instance(size_t sz);
void free_rw_instance(RWInstance* ret);
int rw_instance_get_error(RWInstance* exe, uint32_t* line, const char** function);

#endif
