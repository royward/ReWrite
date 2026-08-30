#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#define ARGREG_NO 8

typedef struct {
// 0
    uint8_t op;
    uint8_t type;
    uint16_t fn_id;
// 4
    uint8_t rule_id;
    uint8_t flags_dst;
    uint8_t flags_src1;
    uint8_t flags_src2;
// 8
    uint32_t dst;
    uint32_t label2;
// 16
    uint64_t src1;
// 24
    uint64_t src2;
// 32
} Operation;

struct Program {
    Operation* code;
    uint32_t instr_max;
    const char* symbols;
    uint32_t label_count;
    uint32_t* labels;
};

typedef struct Program Program;

struct ExecutionState {
    uint32_t sp;
    uint32_t end_of_heap;
    uint64_t* registers;
    uint8_t* overflow;
    Program* program;
    uint32_t errtype;
    uint32_t errline;
    const char* errsym;
    uint64_t argret[ARGREG_NO];
    uint32_t* heap;
};

typedef struct ExecutionState RWInstance;

int program_load(Program* program, const char* filename);
void program_unload(Program* program);
void program_disassemble(Program* program, FILE* out);
int RW__vmcall(RWInstance* a, uint32_t label, uint64_t* inout);
int rw_instance_init(RWInstance* exe, Program* p);
void rw_instance_unload(RWInstance* exe);
int rw_instance_get_error(RWInstance* exe, uint32_t* line, const char** function);

