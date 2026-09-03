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
    union {
        uint64_t src1;
        struct {
            uint32_t src1a;
            uint32_t src1b;
        };
    };
// 24
    union {
        uint64_t src2;
        struct {
            uint32_t src2a;
            uint32_t src2b;
        };
    };
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

uint32_t alloc(RWInstance* exe, uint32_t count, uint32_t size);

static inline uint32_t* rwu_get_header(RWInstance* a, uint32_t x) {return a->heap+(x<<1);}
static inline void* rwu_get_data(uint32_t* header, uint32_t start) {return (void*)(header+4+start);}
static inline uint32_t rwu_get_len(uint32_t* header, uint32_t start) {return (header[1]-start);}

#define UTF32_UNBOUNDED ((size_t)-1)
#define UTF8_ERROR_MALFORMED 8
#define UTF8_ERROR_OVERLONG 9
#define UTF8_ERROR_OUT_OF_BOUNDS 10
#define UTF8_ERROR_SURROGATE 11

int utf32_to_utf8_calc_size(const uint32_t *src, size_t max_len, size_t *out_bytes);
int utf32_to_utf8_convert(const uint32_t *src, size_t max_len, char *dst);
size_t utf8_count_elements(const char* str);
int utf8_populate_buffer(const char* str, uint32_t* dest_buffer);

static inline int utf32_to_string(const uint32_t *src, uint32_t sz, char **dst) {
    size_t size_bytes;
    int err=utf32_to_utf8_calc_size(src,sz,&size_bytes);
    if(err!=0)return err;
    *dst=(char*)malloc(size_bytes+1);
    if(*dst==NULL)return 1;
    err=utf32_to_utf8_convert(src,sz,*dst);
    if(err!=0)free (*dst);
    return err;
}

static inline int string_to_utf32(RWInstance* exe, const char* src, uint32_t* dst, uint32_t* len) {
    size_t count=utf8_count_elements(src);
    uint32_t m=alloc(exe,count,4);
    int err=utf8_populate_buffer(src,exe->heap+m+m+4);
    *dst=m;
    *len=count;
    return err;
}

#define FREE_AND_NULL(p) do { free(*(p)); (*(p)) = NULL; } while(0)

