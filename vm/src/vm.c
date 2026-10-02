#include "vm.h"
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#define REGISTERS_SIZE 10000000
#define LABEL_COUNT 100000
#define HEAP_SIZE 1000000000

#include "defines.inc"

typedef struct {
    uint64_t d0,d1,d2;
    uint32_t d3;
    uint32_t instr_max;
} ProgramHeader;

bool has_op_label(uint16_t op) {
    switch(op) {
#include "oplabels.inc"
        {return true;}
        default: return false;
    }
}

int program_link(Program* program) {
    uint32_t* labels=(uint32_t*)malloc(program->label_count*sizeof(uint32_t));
    if(!labels) {
        fprintf(stderr, "fatal: problem allocating memory\n");
        return EXIT_FAILURE;
    }
    // first get the label offsets
    for(uint32_t i=1;i<program->instr_max;i++) {
        Operation* opv=&program->code[i];
        uint32_t op=opv->op;
        if(op==OP_LABEL) {
            labels[opv->fdst.a]=i;
        }
        if(op==OP_RET || op==OP_CALL || op==OP_CALL_I_O || op==OP_GOTO) {
            i+=opv->flags_dst;
        }

    }
    for(uint32_t i=1;i<program->instr_max;i++) {
        Operation* opv=&program->code[i];
        uint32_t op=opv->op;
        if(op==OP_CALL || op==OP_CALL_I_O || op==OP_GOTO) { // these ones don't have following label
            opv->fdst.a=labels[opv->fdst.a]+1;
        } else if(has_op_label(op)) {
            opv->fdst.a=labels[opv->fdst.a]+1;
            if(opv->fdst.b!=(uint32_t)(-1)) {
                opv->fdst.b=labels[opv->fdst.b]+1;
            }
        }
        if(op==OP_RET || op==OP_CALL || op==OP_CALL_I_O || op==OP_GOTO) {
            i+=opv->flags_dst;
        }
    }
    program->labels=labels;
    return EXIT_SUCCESS;
}

int program_load(Program* program, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if(!f) {
        fprintf(stderr, "fatal: problem loading file\n");
        return EXIT_FAILURE;
    }
    fseek(f, 0, SEEK_END);
    unsigned long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    program->code = malloc(size);
    if(!program->code) {
        fprintf(stderr, "fatal: problem allocating memory\n");
        fclose(f);
        return EXIT_FAILURE;
    }
    fread(program->code, 1, size, f);
    fclose(f);
    ProgramHeader* ph=(ProgramHeader*)program->code;
    program->instr_max = ph->instr_max;
    if(size<program->instr_max*sizeof(Operation)) {
        fprintf(stderr, "fatal: problem loading file - incomplete\n");
        fclose(f);
        return EXIT_FAILURE;
    }
    program->symbols = ((const char*)program->code)+ph->instr_max*sizeof(Operation);
    program->label_count = LABEL_COUNT;
    int ok=program_link(program);
    if(ok) {
        free(program->code);
        fprintf(stderr, "fatal: problem allocating memory\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void program_unload(Program* program) {
    free(program->code);
    free(program->labels);
}

int rw_instance_init(RWInstance* exe, Program* p) {
    exe->program = p;
    exe->registers = (uint64_t*)malloc(REGISTERS_SIZE*sizeof(uint64_t));
    exe->heap8 = (uint64_t*)malloc(HEAP_SIZE*sizeof(uint8_t));
    // create an empty value
    uint32_t* header=rwu_get_header(exe,0);
    header[0]=2;
    header[1]=1;
    header[2]=0;
    header[3]=0xDEADBEEF;
    exe->end_of_heap = 2;
    if(!exe->registers || !exe->heap8) {
        free(exe->registers);
        free(exe->heap8);
        fprintf(stderr, "fatal: problem allocating memory\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void rw_instance_unload(RWInstance* exe) {
    free(exe->registers);
    free(exe->heap8);
    free(exe);
}

#define TYPE_LIST 1
#define TYPE_BOOL 2
#define TYPE_CHAR 3
#define TYPE_I8 4
#define TYPE_U8 5
#define TYPE_I16 6
#define TYPE_U16 7
#define TYPE_I32 8
#define TYPE_U32 9
#define TYPE_I64 10
#define TYPE_U64 11
#define TYPE_I128 12
#define TYPE_U128 13

#define BIND_REG 0
#define BIND_IMM 1

void program_display_label(Program* program, FILE* out, Operation* opv) {
    uint32_t label=0;
    for(uint32_t i=0;i<program->label_count;i++) {
        if(program->labels[i]==opv->fdst.a-1) {
            label=i;
            break;
        }
    }
    fprintf(out," goto %d (line %d)",label,opv->fdst.a);
}

void program_display_label_both(Program* program, FILE* out, Operation* opv) {
    if(opv->fdst.b==(uint32_t)(-1)) {
        program_display_label(program,out,opv);
        return;
    }
    uint32_t label=0;
    for(uint32_t i=0;i<program->label_count;i++) {
        if(program->labels[i]==opv->fdst.a-1) {
            label=i;
            break;
        }
    }
    uint32_t label2=0;
    for(uint32_t i=0;i<program->label_count;i++) {
        if(program->labels[i]==opv->fdst.b-1) {
            label2=i;
            break;
        }
    }
    fprintf(out,"goto %d/%d (line %d/line %d)",label,label2,opv->fdst.a,opv->fdst.b);
}

void display_xreg_assignments(FILE* out, bool outx, const char* r, uint32_t sz, int32_t* p) {
    for(uint32_t i=0;i<sz;i++) {
        uint32_t v=p[i];
        fprintf(out," ");
        if(outx)fprintf(out,"x%d=",i);
        fprintf(out,"%s%d",r,v);
        if(!outx)fprintf(out,"=x%d",i);
    }
}

uint32_t program_disassemble1(Program* program, FILE* out, uint32_t i) {
    Operation* opv=&program->code[i];
    uint32_t ret=0;
    fprintf(out,"%6d %4d %3d: %03x: ",i,opv->fn_id,opv->rule_id,opv->op);
    uint16_t op=opv->op;
    switch(op) {
#include "disassemble.inc"
        case OP_LABEL: {
            fprintf(out,"label %d",opv->fdst.a);
        } break;
        case OP_ERROR: {
            fprintf(out,"error type:");
            fprintf(out,"error_type:%d line:%d sym:%s",opv->fdst.a,opv->fsrc1.a,program->symbols+opv->fsrc2.a);
        } break;
        case OP_RET: {
            uint32_t pc_inc=opv->flags_dst;
            fprintf(out,"ret_exit");
            display_xreg_assignments(out,true,"r",opv->fdst.a,(int32_t*)(&opv->fdst.b));
            ret=pc_inc;
        } break;
        case OP_CALL: {
            uint32_t pc_inc=opv->flags_dst;
            fprintf(out,"call_enter_exit (sp+=%d) ",(int32_t)opv->offset16);
            program_display_label(program,out,opv);
            fprintf(out,"  in:");
            display_xreg_assignments(out,true,"r",opv->fdst.b,(int32_t*)(&opv->fsrc1.b));
            fprintf(out,"  out: %d+off_%d",opv->fsrc1.a,opv->offset16);
            ret=pc_inc;
        } break;
        case OP_CALL_I_O: {
            uint32_t pc_inc=opv->flags_dst;
            fprintf(out,"call_io_ret ");
            program_display_label(program,out,opv);
            fprintf(out,"  in:");
            display_xreg_assignments(out,true,"*r0+",opv->fdst.b,(int32_t*)(&opv->fsrc1.b));
            fprintf(out,"  out:");
            display_xreg_assignments(out,false,"*r0+",opv->fsrc1.a,(int32_t*)(&opv->fsrc1.b)+opv->fdst.b);
            ret=pc_inc;
        } break;
        case OP_GOTO: {
            uint32_t pc_inc=opv->flags_dst;
            fprintf(out,"tail");
            program_display_label(program,out,opv);
            display_xreg_assignments(out,true,"r",opv->fdst.b,(int32_t*)(&opv->fsrc1.a));
            ret=pc_inc;
        } break;
        case OP_LEA: {
            fprintf(out,"let.p r%u = lea r%u",opv->fdst.a,opv->fsrc1.a);
        } break;
        case OP_LEA_SCALE_REG: {
            fprintf(out,"let.p r%u = lea_scale r%u+r%u*%d",opv->fdst.a,opv->fsrc1.a,opv->fsrc2.a,opv->fdst.b);
        } break;
        case OP_LEA_SCALE_IMM: {
            fprintf(out,"let.p r%u = lea_scale r%u+%ld*%d",opv->fdst.a,opv->fsrc1.a,opv->src2,opv->fdst.b);
        } break;
        case OP_ALLOC: {
            fprintf(out,"let r%u = alloc %ld*%ld",opv->fdst.a,opv->src1,opv->src2);
        } break;
        case OP_EMPTY_LIST: {
            fprintf(out,"empty_list (r%u,r%u)",opv->fdst.a,opv->fsrc1.a);
        } break;
        case OP_EXTRACT_LIST: {
            fprintf(out,"extractlist uniq(r%u) (r%u,r%u) = (r%u+%d)",opv->fdst.b,opv->fdst.a,opv->fdst.a+1,opv->fsrc1.a,opv->fsrc1.b);
        } break;
        case OP_DEREF_FREE: {
            fprintf(out,"deref_free r%u",opv->fsrc1.a);
        } break;
        case OP_DEREF_FREE_LIST: {
            fprintf(out,"deref_free_list (r%u,r%u)",opv->fsrc1.a,opv->fsrc1.b);
        } break;
        case OP_REALLOC_VAR: {
            Operation* opv2=&program->code[i+1];
            fprintf(out,"let (r%u,r%u) = realloc_var (r%u,r%u) stride=%ld pre=%d post=%d+r%u",opv->fdst.a,opv2->fdst.a,opv->fsrc1.a,opv2->fsrc1.a,opv->src2,opv->fdst.b,opv2->fdst.b,opv2->fsrc2.a);
        } break;
        case OP_REALLOC_FIXED: {
            Operation* opv2=&program->code[i+1];
            fprintf(out,"let (r%u,r%u) = realloc_fixed (r%u,r%u) stride=%ld pre=%d post=%d",opv->fdst.a,opv2->fdst.a,opv->fsrc1.a,opv2->fsrc1.a,opv->src2,opv->fdst.b,opv2->fdst.b);
        } break;
        case OP_PREPEND_IMM: {
            fprintf(out,"prepend_imm.%d (r%u,r%u+%d) <- %ld",opv->fdst.b,opv->fdst.a,opv->fsrc2.a,opv->fsrc2.b,opv->src1);
        } break;
        case OP_PREPEND_REG: {
            fprintf(out,"prepend_reg.%d (r%u,r%u+%d) <- r%u",opv->fdst.b,opv->fdst.a,opv->fsrc2.a,opv->fsrc2.b,opv->fsrc1.a);
        } break;
        case OP_APPEND_IMM: {
            fprintf(out,"append_imm.%d (r%u) <- %ld",opv->fdst.b,opv->fdst.a,opv->src1);
        } break;
        case OP_APPEND_REG: {
            fprintf(out,"append_reg.%d (r%u) <- r%u",opv->fdst.b,opv->fdst.a,opv->fsrc1.a);
        } break;
        case OP_APPEND_SPLAT: {
            fprintf(out,"append_splat.%d (r%u) <- (r%u,r%u)",opv->fdst.b,opv->fdst.a,opv->fsrc1.a,opv->fsrc2.a);
        } break;
        case OP_APPEND_SPLAT_CONSUME: {
             fprintf(out,"append_splat_consume.%d (r%u) <- (r%u,r%u)",opv->fdst.b,opv->fdst.a,opv->fsrc1.a,opv->fsrc2.a);
        } break;
        case OP_CONT: {
            fprintf(out,"(continuation)");
        } break;
        case OP_INC_MEM: {
            fprintf(out,"inc_mem.%d (r%u+%d) += %d",opv->fsrc1.a, opv->fdst.a, opv->fdst.b, opv->fsrc2.a);
        } break;
        default: fprintf(out,"unknown");
    }
    fprintf(out,"\n");
    return ret;
}

void program_disassemble(Program* program, FILE* out) {
    fprintf(out,"  line func:rule op\n");
    for(uint32_t i=1;i<program->instr_max;i++) {
        i+=program_disassemble1(program,out,i);
    }
}

uint32_t alloc(RWInstance* exe, uint32_t count, uint32_t size) {
    uint64_t sz=((uint64_t)count)*size;
    uint64_t alloc=exe->end_of_heap;
    uint32_t full_size=(16+sz+7)>>3;
    exe->allocated+=full_size;
    //printf("alloc +%d=%d %d\n",full_size,exe->allocated,alloc);
    uint32_t* header=rwu_get_header(exe,alloc);
    header[0]=full_size;
    header[1]=1;
    header[3]=0xDEADBEEF;
    exe->end_of_heap+=full_size;
    return alloc;
}

void deref_free(RWInstance* exe, uint32_t v) {
    uint32_t* header=rwu_get_header(exe,v);
    if(header[1]==0) {
        printf("double free\n");
        exit(1);
    }
    header[1]--;
    if(header[1]==0) {
        uint32_t sz=header[0];
        exe->allocated-=sz;
        //printf("dealloc -%d=%d %d\n",sz,exe->allocated,v);
        for(uint32_t i=1;i<sz;i++) {
            exe->heap8[v+i]=0xDEADBEEFDEADBEEF;
        }
    }
}

static inline uint32_t max_uint32(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}

#if defined(__GNUC__) && !defined(RW_NO_THREADING)
#define RW_THREADED 1
#endif

#ifdef RW_THREADED
  #define CASE(op)     L_##op:
  #define DEFAULT      L_OP_BAD:
  #define DISPATCH()   { opv = opvb++; goto *dispatch_table[opv->op]; }
  #define NEXT()       DISPATCH()
  #if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wgnu-label-as-value"
  #elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
  #endif
#else
  #define CASE(op)     case op:
  #define DEFAULT      default:
  #define DISPATCH()   opv = opvb++; switch (opv->op)
  #define NEXT()       goto dispatch
#endif

int program_execute(RWInstance* exe, uint32_t in_lbl) {
    Program* program=exe->program;
    exe->errtype=0;
    exe->errline=0;
    exe->errsym=NULL;
    uint32_t sp=2; // make sure we are start, even if it was run before
    uint64_t* R=exe->registers+sp;
    //uint32_t pc=program->labels[in_lbl];
    Operation* const code = program->code;
    Operation* opvb = code + program->labels[in_lbl];
    Operation* opv;
#ifdef RW_THREADED
    static const void* const dispatch_table[2048] = {
        #include "oplist.inc"
    };
#endif
#ifndef RW_THREADED
dispatch:
#endif
    DISPATCH()
    {
#include "execute.inc"
            CASE(OP_LABEL) {
            } NEXT();
            CASE(OP_ERROR) {
                exe->errtype=opv->fdst.a;
                exe->errline=opv->src1;
                exe->errsym=program->symbols+opv->src2;
                return exe->errtype;
            };
            CASE(OP_RET) {
                int32_t* p0=(int32_t*)(&opv->fdst.b);
                if(sp==2) {
                    opvb=(Operation*)exe->registers[1];
                    int32_t* p1=(int32_t*)(&opvb->fsrc1.b)+opvb->fdst.b;
                    for(uint32_t index1=0;index1<opvb->fsrc1.a;index1++) {
                        ((uint64_t*)exe->registers[0])[p1[index1]]=R[(p0[index1])];
                    }
                    return 0;
                } else {
                    for(uint32_t index0=0;index0<opv->fdst.a;index0++) {
                        R[index0+1000]=exe->registers[(p0[index0])+sp];
                    }
                    uint64_t* r=&exe->registers[sp-1];
                    opvb=(Operation*)r[0];
                    for(uint32_t index1=0;index1<opvb->fsrc1.a;index1++) {
                        R[index1]=R[index1+1000];
                    }
                    sp-=opvb->offset16; // restore the stack to the old value
                    R=exe->registers+sp;
                    opvb+=opvb->flags_dst+1;
                }
            } NEXT();
            CASE(OP_CALL) {
                uint32_t newsp=sp+(int32_t)opv->offset16;
                int32_t* p=(int32_t*)(&opv->fsrc1.b);
                for(uint32_t index=0;index<opv->fdst.b;index++) {
                    exe->registers[index+newsp]=R[p[index]];
                }
                sp=newsp;
                R=exe->registers+sp;
                uint64_t* r=&exe->registers[sp-1];
                r[0]=(uint64_t)(opv);
                opvb = code + opv->fdst.a;
            } NEXT();
            CASE(OP_CALL_I_O) {
                int32_t* p=(int32_t*)(&opv->fsrc1.b);
                for(uint32_t index=0;index<opv->fdst.b;index++) {
                    R[index]=((uint64_t*)exe->registers[0])[p[index]];
                }
                exe->registers[1]=(uint64_t)(opv);
                opvb = code + opv->fdst.a;
            } NEXT();
            CASE(OP_GOTO) {
                int32_t* p=(int32_t*)(&opv->fsrc1.a);
                for(uint32_t index=0;index<opv->fdst.b;index++) {
                    R[index+1000]=R[p[index]];
                }
                for(uint32_t index=0;index<opv->fdst.b;index++) {
                    R[index]=R[index+1000];
                }
                opvb = code + opv->fdst.a;
            } NEXT();
            CASE(OP_LEA) {
                uint64_t val = R[opv->fsrc1.a];
                R[opv->fdst.a] = ((uint64_t)exe->heap8)+val*8;
            } NEXT();
            CASE(OP_LEA_SCALE_REG) {
                uint64_t val = R[opv->fsrc1.a];
                uint64_t offset = R[opv->fsrc2.a];
                R[opv->fdst.a] = val+offset*opv->fdst.b;
            } NEXT();
            CASE(OP_LEA_SCALE_IMM) {
                uint64_t val = R[opv->fsrc1.a];
                int64_t offset = opv->fsrc2.a;
                R[opv->fdst.a] = val+offset*opv->fdst.b;
            } NEXT();
            CASE(OP_ALLOC) {
                R[opv->fdst.a] = alloc(exe,opv->src1,opv->src2);
            } NEXT();
            CASE(OP_EMPTY_LIST) {
                R[opv->fdst.a]=0;
                R[opv->fsrc1.a]=0;
                rwu_get_header(exe,0)[1]++;
            } NEXT();
            CASE(OP_EXTRACT_LIST) {
                uint32_t* addr=(uint32_t*)(R[opv->fsrc1.a]+opv->fsrc1.b);
                uint64_t v1=addr[0];
                uint64_t v2=addr[1];
                uint32_t* parray=rwu_get_header(exe,R[opv->fdst.b]);
                R[opv->fdst.a]=v1;
                R[opv->fdst.a+1]=v2;
                if(parray[1]==1) { // unique, replace with tombstone
                    addr[0]=0;
                    addr[1]=0;
                    rwu_get_header(exe,0)[1]++;
                } else {
                    rwu_get_header(exe,v1)[1]++;
                }
            } NEXT();
            CASE(OP_REALLOC_VAR) {
                Operation* opv2=opvb;
                uint32_t pre=opv->fdst.b;
                uint32_t post=opv2->fdst.b + R[opv2->fsrc2.a];
                uint32_t array=R[opv->fsrc1.a];
                uint32_t start=R[opv2->fsrc1.a];
                uint32_t stride=opv->src2;
                uint32_t* parray=rwu_get_header(exe,array);
                uint32_t end=parray[2];
                if(parray[1]==1 && pre<=start && (end+post)*stride+16<=(parray[0]<<3)) {
                    R[opv->fdst.a]=array;
                    R[opv2->fdst.a]=start-pre;
                } else {
                    uint32_t sz=max_uint32(pre+post+end-start+2,8);
                    uint32_t full_size=(1<<(64-__builtin_clzll(sz-1)))-2;
                    //printf("realloc %d\n",full_size);
                    uint32_t ret=alloc(exe,full_size,stride);
                    uint32_t* parray2=rwu_get_header(exe,ret);
                    memcpy((uint8_t*)(&parray2[4])+pre*stride,(uint8_t*)(&parray[4])+start*stride,(end-start)*stride);
                    parray2[2]=pre+end;
                    deref_free(exe,array);
                    R[opv->fdst.a]=ret;
                    R[opv2->fdst.a]=0;
                }
                opvb++;
            } NEXT();
            CASE(OP_REALLOC_FIXED) {
                Operation* opv2=opvb;
                uint32_t pre=opv->fdst.b;
                uint32_t post=opv2->fdst.b;
                uint32_t array=R[opv->fsrc1.a];
                uint32_t start=R[opv2->fsrc1.a];
                uint32_t stride=opv->src2;
                uint32_t* parray=rwu_get_header(exe,array);
                uint32_t end=parray[2];
                if(parray[1]==1 && pre<=start && (end+post)*stride+16<=(parray[0]<<3)) {
                    R[opv->fdst.a]=array;
                    R[opv2->fdst.a]=start-pre;
                } else {
                    uint32_t sz=max_uint32(pre+post+end-start+2,8);
                    uint32_t full_size=(1<<(64-__builtin_clzll(sz-1)))-2;
                    uint32_t ret=alloc(exe,full_size,stride);
                    uint32_t* parray2=rwu_get_header(exe,ret);
                    //printf("realloc %lx <= %lx sz=%d\n",(uint8_t*)(&parray2[4])+pre*stride,(uint8_t*)(&parray[4])+start*stride,(end-start)*stride);
                    memcpy((uint8_t*)(&parray2[4])+pre*stride,(uint8_t*)(&parray[4])+start*stride,(end-start)*stride);
                    parray2[2]=pre+end;
                    deref_free(exe,array);
                    R[opv->fdst.a]=ret;
                    R[opv2->fdst.a]=0;
                }
                opvb++;
            } NEXT();
            CASE(OP_PREPEND_IMM) {
                uint32_t array=R[opv->fdst.a];
                uint32_t start=R[opv->fsrc2.a];
                uint32_t value=opv->src1;
                uint32_t offset=opv->fsrc2.b;
                uint32_t* parray=rwu_get_header(exe,array);
                uint8_t* addr=((uint8_t*)parray)+16+(start+offset)*(opv->fdst.b>>3);
                switch(opv->fdst.b) { // alignment is guaranteed by the compiler
                    case 1:case 8:*((uint8_t*)addr)=(uint8_t)value; break;
                    case 16:*((uint16_t*)addr)=(uint16_t)value; break;
                    case 32:*((uint32_t*)addr)=(uint32_t)value; break;
                    case 64:*((uint64_t*)addr)=(uint64_t)value; break;
                    default: fprintf(stderr,"unknown size in append\n"); exit(EXIT_FAILURE);
                } NEXT();
            } NEXT();
           CASE(OP_PREPEND_REG) {
                uint32_t array=R[opv->fdst.a];
                uint32_t start=R[opv->fsrc2.a];
                uint32_t value=R[opv->fsrc1.a];
                uint32_t offset=opv->fsrc2.b;
                uint32_t* parray=rwu_get_header(exe,array);
                uint8_t* addr=((uint8_t*)parray)+16+(start+offset)*(opv->fdst.b>>3);
                switch(opv->fdst.b) { // alignment is guaranteed by the compiler
                    case 1:case 8:*((uint8_t*)addr)=(uint8_t)value; break;
                    case 16:*((uint16_t*)addr)=(uint16_t)value; break;
                    case 32:*((uint32_t*)addr)=(uint32_t)value; break;
                    case 64:*((uint64_t*)addr)=(uint64_t)value; break;
                    default: fprintf(stderr,"unknown size in append\n"); exit(EXIT_FAILURE);
                } NEXT();
            } NEXT();
            CASE(OP_APPEND_IMM) {
                uint32_t array=R[opv->fdst.a];
                uint32_t value=opv->src1;
                uint32_t* parray=rwu_get_header(exe,array);
                uint32_t end=parray[2];
                parray[2]=end+1;
                uint8_t* addr=((uint8_t*)parray)+16+end*(opv->fdst.b>>3);
                switch(opv->fdst.b) { // alignment is guaranteed by the compiler
                    case 1:case 8:*((uint8_t*)addr)=(uint8_t)value; break;
                    case 16:*((uint16_t*)addr)=(uint16_t)value; break;
                    case 32:*((uint32_t*)addr)=(uint32_t)value; break;
                    case 64:*((uint64_t*)addr)=(uint64_t)value; break;
                    default: fprintf(stderr,"unknown size in append\n"); exit(EXIT_FAILURE);
                } NEXT();
            } NEXT();
            CASE(OP_APPEND_REG) {
                uint32_t array=R[opv->fdst.a];
                uint32_t value=R[opv->fsrc1.a];
                uint32_t* parray=rwu_get_header(exe,array);
                uint32_t end=parray[2];
                parray[2]=end+1;
                uint8_t* addr=((uint8_t*)parray)+16+end*(opv->fdst.b>>3);
                switch(opv->fdst.b) { // alignment is guaranteed by the compiler
                    case 1:case 8:*((uint8_t*)addr)=(uint8_t)value; break;
                    case 16:*((uint16_t*)addr)=(uint16_t)value; break;
                    case 32:*((uint32_t*)addr)=(uint32_t)value; break;
                    case 64:*((uint64_t*)addr)=(uint64_t)value; break;
                    default: fprintf(stderr,"unknown size in append\n"); exit(EXIT_FAILURE);
                } NEXT();
            } NEXT();
            CASE(OP_APPEND_SPLAT) {
                uint32_t dstarray=R[opv->fdst.a];
                uint32_t array=R[opv->fsrc1.a];
                uint32_t start=R[opv->fsrc2.a];
                uint32_t* pdstarray=rwu_get_header(exe,dstarray);
                uint32_t* parray=rwu_get_header(exe,array);
                uint32_t dstend=pdstarray[2];
                uint32_t end=parray[2];
                uint32_t stride=opv->fdst.b>>3;
                uint8_t* dstaddr=((uint8_t*)pdstarray)+16+dstend*(opv->fdst.b>>3);
                uint8_t* addr=((uint8_t*)parray)+16+start*(opv->fdst.b>>3);
                uint32_t len=end-start;
                memcpy(dstaddr,addr,len*stride);
                pdstarray[2]=dstend+len;
            } NEXT();
            CASE(OP_APPEND_SPLAT_CONSUME) {
                uint32_t dstarray=R[opv->fdst.a];
                uint32_t array=R[opv->fsrc1.a];
                uint32_t start=R[opv->fsrc2.a];
                uint32_t* pdstarray=rwu_get_header(exe,dstarray);
                uint32_t* parray=rwu_get_header(exe,array);
                uint32_t dstend=pdstarray[2];
                uint32_t end=parray[2];
                uint32_t stride=opv->fdst.b>>3;
                uint8_t* dstaddr=((uint8_t*)pdstarray)+16+dstend*(opv->fdst.b>>3);
                uint8_t* addr=((uint8_t*)parray)+16+start*(opv->fdst.b>>3);
                uint32_t len=end-start;
                memcpy(dstaddr,addr,len*stride);
                pdstarray[2]=dstend+len;
                deref_free(exe,array);
            } NEXT();
            CASE(OP_DEREF_FREE) {
                uint32_t array=R[opv->fsrc1.a];
                deref_free(exe,array);
            } NEXT();
            CASE(OP_DEREF_FREE_LIST) {
                uint32_t array=R[opv->fsrc1.a];
                //uint32_t start=R[opv->fsrc2.b];
                uint32_t* parray=rwu_get_header(exe,array);
                uint32_t* data=rwu_get_data(parray);
                uint32_t end=parray[2];
                for(uint32_t i=0;i<end;i++) {
                    deref_free(exe,data[i+i]);
                }
                deref_free(exe,array);
            } NEXT();
           CASE(OP_INC_MEM) {
                uint8_t* addr=(uint8_t*)(R[opv->fdst.a]+opv->fdst.b);
                uint32_t sz=opv->fsrc1.a;
                switch(sz) { // alignment is guaranteed by the compiler
                    case 1:case 8:*((uint8_t*)addr)+=(uint8_t)opv->fsrc2.a; break;
                    case 16:*((uint16_t*)addr)+=(uint16_t)opv->fsrc2.a; break;
                    case 32:*((uint32_t*)addr)+=(uint32_t)opv->fsrc2.a; break;
                    case 64:*((uint64_t*)addr)+=(uint64_t)opv->fsrc2.a; break;
                    default: fprintf(stderr,"unknown size in operand_store\n"); exit(EXIT_FAILURE);
                }
            } NEXT();
            CASE(OP_ZEXT) {
            } NEXT();
            CASE(OP_CONT) {
            } NEXT();
            DEFAULT {
                fprintf(stderr,"Illegal Instruction %x\n",opv->op);
                exe->errtype=1;
                return exe->errtype;
            }
        }
}

#ifdef RW_THREADED
#if defined(__clang__)
  #pragma clang diagnostic pop
#elif defined(__GNUC__)
  #pragma GCC diagnostic pop
#endif
#endif

int RW__vmcall(RWInstance* a, uint32_t label, uint64_t* inout) {
    a->registers[0]=(uint64_t)inout;
    program_execute(a,label);
    return a->errtype;
}

int rw_instance_get_error(RWInstance* exe, uint32_t* line, const char** function) {
    int err=exe->errtype;
    if(err!=0) {
        *line=exe->errline;
        *function=exe->errsym;
    }
    return err;
}

// Returns 0 on success, -1 if an invalid code point / surrogate is found. Output size via out_bytes.
int utf32_to_utf8_calc_size(const uint32_t *src, size_t max_len, size_t *out_bytes) {
    size_t bytes = 0;
    size_t i = 0;
    while ((max_len == UTF32_UNBOUNDED ? src[i] != 0 : i < max_len)) {
        uint32_t cp = src[i++];
        if (cp <= 0x7F) {
            bytes += 1;
        } else if(cp <= 0x7FF) {
            bytes += 2;
        } else if (cp <= 0xFFFF) {
            if(cp >= 0xD800 && cp <= 0xDFFF) {
                return -1; // Reject UTF-16 surrogates
            }
            bytes += 3;
        }
        else if(cp <= 0x10FFFF) {
            bytes += 4;
        }
        else {
            return -1; // Out of Unicode bounds
        }
    }
    *out_bytes = bytes;
    return 0;
}

// Returns 0 on success, -1 if validation fails.
int utf32_to_utf8_convert(const uint32_t *src, size_t max_len, char *dst) {
    uint8_t *d = (uint8_t *)dst;
    size_t i = 0;
    while ((max_len == UTF32_UNBOUNDED ? src[i] != 0 : i < max_len)) {
        uint32_t cp = src[i++];
        if (cp > 0x10FFFF) return UTF8_ERROR_OUT_OF_BOUNDS;
        if (cp >= 0xD800 && cp <= 0xDFFF) return UTF8_ERROR_SURROGATE;
        if (cp <= 0x7F) {
            *d++ = (uint8_t)cp;
        } else if (cp <= 0x7FF) {
            *d++ = (uint8_t)(0xC0 | (cp >> 6));
            *d++ = (uint8_t)(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            if (cp >= 0xD800 && cp <= 0xDFFF) return -1;
            *d++ = (uint8_t)(0xE0 | (cp >> 12));
            *d++ = (uint8_t)(0x80 | ((cp >> 6) & 0x3F));
            *d++ = (uint8_t)(0x80 | (cp & 0x3F));
        } else if (cp <= 0x10FFFF) {
            *d++ = (uint8_t)(0xF0 | (cp >> 18));
            *d++ = (uint8_t)(0x80 | ((cp >> 12) & 0x3F));
            *d++ = (uint8_t)(0x80 | ((cp >> 6) & 0x3F));
            *d++ = (uint8_t)(0x80 | (cp & 0x3F));
        } else {
            return -1;
        }
    }
    //if (max_len == UTF32_UNBOUNDED || src[i] == 0) {
        *d = '\0';
    //}
    return 0;
}

size_t utf8_count_elements(const char* str) {
    const uint8_t* s = (const uint8_t*)str;
    size_t length = 0;
    while (*s != '\0') {
        if ((*s & 0xC0) != 0x80) {
            length++;
        }
        s++;
    }
    return length;
}

int utf8_populate_buffer(const char* str, uint32_t* dest_buffer) {
    const uint8_t* s = (const uint8_t*)str;
    size_t index = 0;
    while (*s != '\0') {
        uint32_t c = *s;
        // Case 1: Ultra-fast ASCII Path (1 byte)
        if (c < 0x80) {
            dest_buffer[index++] = c;
            s++;
            continue;
        }
        uint32_t cp;
        // Case 2: 2-Byte Sequence (110xxxxx 10xxxxxx)
        if ((c & 0xE0) == 0xC0) {
            if ((s[1] & 0xC0) != 0x80) return UTF8_ERROR_MALFORMED;
            cp = ((c & 0x1F) << 6) | (s[1] & 0x3F);
            if (cp < 0x80) return UTF8_ERROR_OVERLONG; // Overlong check
            dest_buffer[index++] = cp;
            s += 2;
        }
        // Case 3: 3-Byte Sequence (1110xxxx 10xxxxxx 10xxxxxx)
        else if ((c & 0xF0) == 0xE0) {
            if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) return UTF8_ERROR_MALFORMED;
            cp = ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
            if (cp < 0x0800) return UTF8_ERROR_OVERLONG;
            if (cp >= 0xD800 && cp <= 0xDFFF) return UTF8_ERROR_SURROGATE;
            dest_buffer[index++] = cp;
            s += 3;
        }
        // Case 4: 4-Byte Sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        else if ((c & 0xF8) == 0xF0) {
            if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 || (s[3] & 0xC0) != 0x80) return UTF8_ERROR_MALFORMED;
            cp = ((c & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
            if (cp < 0x010000) return UTF8_ERROR_OVERLONG;
            if (cp > 0x10FFFF) return UTF8_ERROR_OUT_OF_BOUNDS;
            dest_buffer[index++] = cp;
            s += 4;
        }
        else {
            return UTF8_ERROR_MALFORMED;
        }
    }
    return 0;
}
