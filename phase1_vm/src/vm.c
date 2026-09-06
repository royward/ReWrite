#include "vm.h"
#include <stdlib.h>
#include <inttypes.h>

#define OVERFLOW_SIZE 1000
#define REGISTERS_SIZE 100000
#define LABEL_COUNT 100000
#define HEAP_SIZE 1000000

#define OP_LABEL 0x01
#define OP_MIN_BRANCH 0xF0

typedef struct {
    uint64_t d0,d1,d2;
    uint32_t d3;
    uint32_t instr_max;
} ProgramHeader;

int program_link(Program* program) {
    uint32_t* labels=(uint32_t*)malloc(program->label_count*sizeof(uint32_t));
    if(!labels) {
        fprintf(stderr, "fatal: problem allocating memory\n");
        return EXIT_FAILURE;
    }
    // first get the label offsets
    for(uint32_t i=1;i<program->instr_max;i++) {
        Operation* operation=&program->code[i];
        if(operation->op==OP_LABEL) {
            labels[operation->dst]=i;
        }
    }
    for(uint32_t i=1;i<program->instr_max;i++) {
        Operation* operation=&program->code[i];
        if(operation->op>=OP_MIN_BRANCH) {
            operation->dst=labels[operation->dst];
            if(operation->label2!=(uint32_t)(-1)) {
                operation->label2=labels[operation->label2];
            }
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
    exe->overflow = (uint8_t*)malloc(OVERFLOW_SIZE*sizeof(uint8_t));
    exe->heap = (uint32_t*)malloc(HEAP_SIZE*sizeof(uint8_t));
    exe->end_of_heap = 0;
    if(!exe->registers || !exe->overflow || !exe->heap) {
        free(exe->registers);
        free(exe->overflow);
        free(exe->heap);
        fprintf(stderr, "fatal: problem allocating memory\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void rw_instance_unload(RWInstance* exe) {
    free(exe->registers);
    free(exe->overflow);
        free(exe->heap);
    free(exe);
}

#define OP_LABEL 0x01
#define OP_ERROR 0x02
#define OP_RET 0x03
#define OP_ADD_STACK 0x05
#define OP_LLALLOC 0x0E
#define OP_STORE 0x0F
#define OP_MOVE 0x10
#define OP_PLUS 0x18
#define OP_MINUS 0x19
#define OP_TIMES 0x1A
#define OP_DIVIDE 0x1B
#define OP_MODULUS 0x1C
#define OP_LT 0x1D
#define OP_LTE 0x1E
#define OP_INSERT_LL 0x20
#define OP_LEA 0x28
#define OP_LEA_SCALE 0x29
#define OP_EXTRACT_LL 0x30
#define OP_ALLOC 0xC0
#define OP_CMP_NE_BRANCH 0xF0
#define OP_CMP_LT_BRANCH 0xF6
#define OP_CMP_LE_BRANCH 0xF7
#define OP_CMP_EQ_BRANCH 0xF8
#define OP_GOTO 0xFE
#define OP_CALL 0xFF

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

#define BIND_IMM 0
#define BIND_REG 1
#define BIND_OVERFLOW 2
#define BIND_ARGRET 3
#define BIND_INOUT 4
#define BIND_MEM 5

#define FLAG_LIFE_START 0x10
#define FLAG_LIFE_END 0x20

void display_operand(FILE* out, uint8_t flag, int64_t val) {
    uint8_t bind=flag&0xF;
    if(flag&FLAG_LIFE_START) {
        fprintf(out,"+");
    } else if(flag&FLAG_LIFE_END) {
        fprintf(out,"-");
    }
    switch(bind) {
        case BIND_IMM: {
            fprintf(out,"%" PRId64,val);
        } break;
        case BIND_REG: {
            fprintf(out,"r%" PRIu64,val);
        } break;
        case BIND_ARGRET: {
            fprintf(out,"x%" PRIu64,val);
        } break;
        case BIND_OVERFLOW: {
            fprintf(out,"ov+%" PRId64,val);
        } break;
        case BIND_INOUT: {
            fprintf(out,"(r0+%" PRId64 ")",val);
        } break;
        case BIND_MEM: {
            fprintf(out,"(r%" PRId64 "+%" PRId64 ")",val&0xFFFFFFFF, val>>32);
        } break;
    }
}

void program_display_label(Program* program, FILE* out, Operation* operation) {
    uint32_t label=0;
    for(uint32_t i=0;i<program->label_count;i++) {
        if(program->labels[i]==operation->dst) {
            label=i;
            break;
        }
    }
    fprintf(out,"%d (line %d)",label,operation->dst);
}

void program_display_label_both(Program* program, FILE* out, Operation* operation) {
    if(operation->label2==(uint32_t)(-1)) {
        program_display_label(program,out,operation);
        return;
    }
    uint32_t label=0;
    for(uint32_t i=0;i<program->label_count;i++) {
        if(program->labels[i]==operation->dst) {
            label=i;
            break;
        }
    }
    uint32_t label2=0;
    for(uint32_t i=0;i<program->label_count;i++) {
        if(program->labels[i]==operation->label2) {
            label2=i;
            break;
        }
    }
    fprintf(out,"%d/%d (line %d/line %d)",label,label2,operation->dst,operation->label2);
}

const char* display_type(uint8_t tp) {
    switch(tp) {
        case TYPE_BOOL: return "bool"; break;
        case TYPE_CHAR: return "char"; break;
        case TYPE_I8: return "i8"; break;
        case TYPE_U8: return "u8"; break;
        case TYPE_I16: return "i16"; break;
        case TYPE_U16: return "u16"; break;
        case TYPE_I32: return "i32"; break;
        case TYPE_U32: return "u32"; break;
        case TYPE_I64: return "i64"; break;
        case TYPE_U64: return "u64"; break;
        case TYPE_I128: return "i128"; break;
        case TYPE_U128: return "u128"; break;
        default: return "unknown";
    }
}

uint32_t type_to_size(uint8_t tp) {
    switch(tp) {
        case TYPE_BOOL: return 1; break;
        case TYPE_CHAR: return 32; break;
        case TYPE_I8: return 8; break;
        case TYPE_U8: return 8; break;
        case TYPE_I16: return 16; break;
        case TYPE_U16: return 16; break;
        case TYPE_I32: return 32; break;
        case TYPE_U32: return 32; break;
        case TYPE_I64: return 64; break;
        case TYPE_U64: return 64; break;
        case TYPE_I128: return 128; break;
        case TYPE_U128: return 128; break;
        default: return 0;
    }
}

const char* display_binop(uint8_t op) {
    switch(op) {
        case OP_PLUS: return "+"; break;
        case OP_MINUS: return "-"; break;
        case OP_TIMES: return "*"; break;
        case OP_DIVIDE: return "/"; break;
        case OP_LT: return "<"; break;
        case OP_LTE: return "<="; break;
        default: return "(unknown)";
    }
}

void program_disassemble(Program* program, FILE* out) {
    fprintf(out,"  line func:rule op\n");
    for(uint32_t i=1;i<program->instr_max;i++) {
        Operation* operation=&program->code[i];
        fprintf(out,"%6d %4d %3d: %02x: ",i,operation->fn_id,operation->rule_id,operation->op);
        uint8_t op=operation->op;
        switch(op) {
            case OP_LABEL: {
                fprintf(out,"label %d",operation->dst);
            } break;
            case OP_LLALLOC: {
                fprintf(out,"ll_alloc %d",operation->dst);
            } break;
            case OP_STORE: {
                fprintf(out,"ll_store.%s %d",display_type(operation->type),operation->dst);
            } break;
            case OP_ERROR: {
                fprintf(out,"error type:");
                display_operand(out,operation->flags_dst,operation->dst);
                fprintf(out," line:%ld sym:%s",operation->src1,program->symbols+operation->src2);
            } break;
            case OP_RET: {
                fprintf(out,"ret");
            } break;
            case OP_CALL: {
                fprintf(out,"call (sp+=%d) ",(int32_t)operation->src1);
                program_display_label(program,out,operation);
                if(operation->src2>0) {
                    fprintf(out," (o=%d)",(uint32_t)operation->src2);
                }
            } break;
            case OP_GOTO: {
                fprintf(out,"goto ");
                program_display_label(program,out,operation);
                if(operation->src2>0) {
                    fprintf(out," (o=%d)",(uint32_t)operation->src2);
                }
            } break;
            case OP_ADD_STACK: {
                if((int32_t)operation->src1>=0) {
                    fprintf(out,"sp+=%d ",(int32_t)operation->src1);
                } else {
                    fprintf(out,"sp-=%d ",-(int32_t)operation->src1);
                }
            } break;
            case OP_LEA: {
                fprintf(out,"let.p ");
                display_operand(out,operation->flags_dst,operation->dst);
                fprintf(out," = lea ");
                display_operand(out,operation->flags_src1,operation->src1);
            } break;
           case OP_LEA_SCALE: {
                fprintf(out,"let.p ");
                display_operand(out,operation->flags_dst,operation->dst);
                fprintf(out," = lea_scale ");
                display_operand(out,operation->flags_src1,operation->src1);
                fprintf(out,"+");
                display_operand(out,operation->flags_src2,operation->src2);
                fprintf(out,"*%d",operation->label2);
            } break;
            case OP_ALLOC: {
                fprintf(out,"let ");
                display_operand(out,operation->flags_dst,operation->dst);
                fprintf(out," = alloc ");
                display_operand(out,operation->flags_src1,operation->src1);
                fprintf(out,"*");
                display_operand(out,operation->flags_src2,operation->src2);
            } break;
            case OP_MOVE: case OP_MOVE+1: case OP_MOVE+2: case OP_MOVE+3: case OP_MOVE+4: {
                uint32_t sz=(op&7)==0?1:(8<<((op-1)&7));
                fprintf(out,"let.%d ",sz);
                display_operand(out,operation->flags_dst,operation->dst);
                fprintf(out," = ");
                display_operand(out,operation->flags_src1,operation->src1);
            } break;
            case OP_INSERT_LL: case OP_INSERT_LL+1: case OP_INSERT_LL+2: case OP_INSERT_LL+3: case OP_INSERT_LL+4: {
                uint32_t sz=(op&7)==0?1:(8<<((op-1)&7));
                fprintf(out,"insert.%d ",sz);
                display_operand(out,operation->flags_dst,operation->dst);
                fprintf(out," = ");
                display_operand(out,operation->flags_src1,operation->src1);
                fprintf(out," (o=%d)",(uint32_t)operation->src2);
            } break;
            case OP_EXTRACT_LL: case OP_EXTRACT_LL+1: case OP_EXTRACT_LL+2: case OP_EXTRACT_LL+3: case OP_EXTRACT_LL+4: {
                uint32_t sz=(op&7)==0?1:(8<<((op-1)&7));
                fprintf(out,"extract.%d ",sz);
                display_operand(out,operation->flags_dst,operation->dst);
                fprintf(out," = ");
                display_operand(out,operation->flags_src1,operation->src1);
                fprintf(out," (o=%d)",(uint32_t)operation->src2);
            } break;
            case OP_CMP_NE_BRANCH: case OP_CMP_NE_BRANCH+1: case OP_CMP_NE_BRANCH+2: case OP_CMP_NE_BRANCH+3: case OP_CMP_NE_BRANCH+4: {
                uint32_t sz=(op&7)==0?1:(8<<((op-1)&7));
                fprintf(out,"test.%d ",sz);
                display_operand(out,operation->flags_src1,operation->src1);
                fprintf(out," != ");
                display_operand(out,operation->flags_src2,operation->src2);
                fprintf(out," goto ");
                program_display_label_both(program,out,operation);
            } break;
            case OP_CMP_EQ_BRANCH: case OP_CMP_EQ_BRANCH+1: case OP_CMP_EQ_BRANCH+2: case OP_CMP_EQ_BRANCH+3: case OP_CMP_EQ_BRANCH+4: {
                uint32_t sz=(op&7)==0?1:(8<<((op-1)&7));
                fprintf(out,"test.%d ",sz);
                display_operand(out,operation->flags_src1,operation->src1);
                fprintf(out," == ");
                display_operand(out,operation->flags_src2,operation->src2);
                fprintf(out," goto ");
                program_display_label_both(program,out,operation);
            } break;
            case OP_CMP_LT_BRANCH: {
                fprintf(out,"test.%s ",display_type(operation->type));
                display_operand(out,operation->flags_src1,operation->src1);
                fprintf(out," < ");
                display_operand(out,operation->flags_src2,operation->src2);
                fprintf(out," goto ");
                program_display_label_both(program,out,operation);
            } break;
            case OP_CMP_LE_BRANCH: {
                fprintf(out,"test.%s ",display_type(operation->type));
                display_operand(out,operation->flags_src1,operation->src1);
                fprintf(out," <= ");
                display_operand(out,operation->flags_src2,operation->src2);
                fprintf(out," goto ");
                program_display_label_both(program,out,operation);
            } break;
            case OP_PLUS: case OP_MINUS: case OP_TIMES: case OP_DIVIDE: case OP_MODULUS: case OP_LT: case OP_LTE: {
                fprintf(out,"let.%s ",display_type(operation->type));
                display_operand(out,operation->flags_dst,operation->dst);
                fprintf(out," = ");
                display_operand(out,operation->flags_src1,operation->src1);
                fprintf(out," %s ",display_binop(op));
                display_operand(out,operation->flags_src2,operation->src2);
            } break;
            default: fprintf(out,"unknown");
        }
        fprintf(out,"\n");
    }
}

void operand_store(RWInstance* exe, Operation* operation, uint64_t value, uint32_t sz, uint32_t sp) {
    switch(operation->flags_dst&0xF) {
        case BIND_IMM: {
            fprintf(stderr,"Fatal error trying to store to an immediate\n");
            exit(EXIT_FAILURE);
        } break;
        case BIND_REG: {
            value&=(sz>=64)?(uint64_t)-1LL:(uint64_t)((1LL<<sz)-1);
            exe->registers[operation->dst+sp]=value;
        } break;
        case BIND_ARGRET: {
            value&=(sz>=64)?(uint64_t)-1LL:(uint64_t)((1LL<<sz)-1);
            exe->argret[operation->dst]=value;
        } break;
        case BIND_INOUT: {
            value&=(sz>=64)?(uint64_t)-1LL:(uint64_t)((1LL<<sz)-1);
            ((uint64_t*)exe->registers[0])[operation->dst]=value;
        } break;
        case BIND_OVERFLOW: {
            uint8_t* addr=&exe->overflow[operation->dst];
            switch(sz) { // alignment is guaranteed by the compiler
                case 1:case 8:*((uint8_t*)addr)=(uint8_t)value; break;
                case 16:*((uint16_t*)addr)=(uint16_t)value; break;
                case 32:*((uint32_t*)addr)=(uint32_t)value; break;
                case 64:*((uint64_t*)addr)=(uint64_t)value; break;
                default: fprintf(stderr,"unknown size in operand_store\n"); exit(EXIT_FAILURE);
            }
        } break;
        case BIND_MEM: {
            uint8_t* addr=(uint8_t*)(exe->registers[operation->dst+sp]+operation->label2);
            switch(sz) { // alignment is guaranteed by the compiler
                case 1:case 8:*((uint8_t*)addr)=(uint8_t)value; break;
                case 16:*((uint16_t*)addr)=(uint16_t)value; break;
                case 32:*((uint32_t*)addr)=(uint32_t)value; break;
                case 64:*((uint64_t*)addr)=(uint64_t)value; break;
                default: fprintf(stderr,"unknown size in operand_store\n"); exit(EXIT_FAILURE);
            }
        } break;
        default: {
            fprintf(stderr,"unknown flag in operand_store\n");
            exit(EXIT_FAILURE);
        }
    }
}

uint64_t operand_load(RWInstance* exe, uint32_t sz, uint32_t flags_src, uint64_t src, uint32_t sp) {
    uint64_t mask=(sz>=64)?(uint64_t)-1LL:(uint64_t)((1LL<<sz)-1);
    switch(flags_src&0xF) {
        case BIND_IMM: {
            return src&mask;
        } break;
        case BIND_REG: {
            return exe->registers[src+sp]&mask;
        } break;
        case BIND_ARGRET: {
            return exe->argret[src]&mask;
        } break;
        case BIND_INOUT: {
            return ((uint64_t*)exe->registers[0])[src]&mask;
        } break;
        case BIND_OVERFLOW: {
            uint8_t* addr=&exe->overflow[src];
            uint64_t value;
            switch(sz) { // alignment is guaranteed by the compiler
                case 1:case 8: value=*((uint8_t*)addr); break;
                case 16:value=*((uint16_t*)addr); break;
                case 32:value=*((uint32_t*)addr); break;
                case 64:value=*((uint64_t*)addr); break;
                default: fprintf(stderr,"unknown size in operand_load\n"); exit(EXIT_FAILURE);
            }
            return value&mask;
        } break;
        case BIND_MEM: {
            uint8_t* addr=(uint8_t*)(exe->registers[(src&0xFFFFFFFF)+sp]+(src>>32));
            uint64_t value;
            switch(sz) { // alignment is guaranteed by the compiler
                case 1:case 8: value=*((uint8_t*)addr); break;
                case 16:value=*((uint16_t*)addr); break;
                case 32:value=*((uint32_t*)addr); break;
                case 64:value=*((uint64_t*)addr); break;
                default: fprintf(stderr,"unknown size in operand_load\n"); exit(EXIT_FAILURE);
            }
            return value&mask;
        } break;
        default: {
            fprintf(stderr,"unknown flag in operand_load\n");
            exit(EXIT_FAILURE);
        }
    }
}

uint32_t alloc(RWInstance* exe, uint32_t count, uint32_t size) {
    uint64_t sz=((uint64_t)count)*size;
    uint64_t alloc=exe->end_of_heap;
    uint32_t full_size=(16+sz+7)>>3;
    exe->heap[alloc+alloc]=full_size;
    exe->heap[alloc+alloc+2]=1;
    exe->heap[alloc+alloc+3]=0xDEADBEEF;
    exe->end_of_heap+=full_size;
    return alloc;
}

int program_execute(RWInstance* exe, uint32_t in_lbl) {
    Program* program=exe->program;
    exe->errtype=0;
    exe->errline=0;
    exe->errsym=NULL;
    uint32_t sp=0; // make sure we are start, even if it was run before
    uint32_t pc=program->labels[in_lbl];
    while(true) {
        Operation* operation=&program->code[pc];
        uint8_t op=operation->op;
        switch(op) {
            case OP_LABEL: {
            } break;
            case OP_LLALLOC:
            case OP_STORE: {
                // NOP
            } break;
            case OP_ERROR: {
                exe->errtype=operand_load(exe, 64, operation->flags_dst, operation->dst, sp);
                exe->errline=operation->src1;
                exe->errsym=program->symbols+operation->src2;
                return exe->errtype;
            };
            case OP_RET: {
                if(sp==0) {
                    return 0;
                } else {
                    pc=exe->registers[sp-1];
                }
            } break;
            case OP_CALL: {
                sp+=(int32_t)operation->src1;
                exe->registers[sp-1]=pc;
                pc = operation->dst-1;
            } break;
            case OP_GOTO: {
                pc = operation->dst-1;
            } break;
            case OP_ADD_STACK: {
                 sp+=(int32_t)operation->src1;
            } break;
            case OP_LEA: {
                uint64_t val = operand_load(exe, 64, operation->flags_src1, operation->src1, sp);
                operand_store(exe, operation, ((uint64_t)exe->heap)+val*8, 64, sp);
            } break;
            case OP_LEA_SCALE: {
                uint64_t val = operand_load(exe, 64, operation->flags_src1, operation->src1, sp);
                uint64_t offset = operand_load(exe, 32, operation->flags_src2, operation->src2, sp);
                operand_store(exe, operation, val+offset*operation->label2, 64, sp);
            } break;
            case OP_ALLOC: {
                uint32_t ret=alloc(exe,operand_load(exe,64,operation->flags_src1,operation->src1,sp),operand_load(exe,64,operation->flags_src1,operation->src2,sp));
                operand_store(exe, operation, ret, 64, sp);
            } break;
            case OP_MOVE: case OP_INSERT_LL: case OP_EXTRACT_LL: {
                uint64_t val = operand_load(exe, 1, operation->flags_src1, operation->src1, sp);
                operand_store(exe, operation, val, 1, sp);
            }
            case OP_MOVE+1: case OP_MOVE+2: case OP_MOVE+3: case OP_MOVE+4:
            case OP_INSERT_LL+1: case OP_INSERT_LL+2: case OP_INSERT_LL+3: case OP_INSERT_LL+4:
            case OP_EXTRACT_LL+1: case OP_EXTRACT_LL+2: case OP_EXTRACT_LL+3: case OP_EXTRACT_LL+4: {
                uint32_t sz=(op&7)==0?1:(8<<((op-1)&7));
                uint64_t val = operand_load(exe, sz, operation->flags_src1, operation->src1, sp);
                operand_store(exe, operation, val, sz, sp);
            } break;
            case OP_CMP_NE_BRANCH: {
                uint64_t src1 = operand_load(exe, 1, operation->flags_src1, operation->src1, sp);
                uint64_t src2 = operand_load(exe, 1, operation->flags_src2, operation->src2, sp);
                if(src1 != src2) {
                    pc = operation->dst-1; // -1 because pc++ at end of loop
                }
            } break;
            case OP_CMP_NE_BRANCH+1: case OP_CMP_NE_BRANCH+2: case OP_CMP_NE_BRANCH+3: case OP_CMP_NE_BRANCH+4: {
                uint32_t sz=(op&7)==0?1:(8<<((op-1)&7));
                uint64_t src1 = operand_load(exe, sz, operation->flags_src1, operation->src1, sp);
                uint64_t src2 = operand_load(exe, sz, operation->flags_src2, operation->src2, sp);
                if(src1 != src2) {
                    pc = operation->dst-1; // -1 because pc++ at end of loop
                }
            } break;
            case OP_CMP_EQ_BRANCH: {
                uint64_t src1 = operand_load(exe, 1, operation->flags_src1, operation->src1, sp);
                uint64_t src2 = operand_load(exe, 1, operation->flags_src2, operation->src2, sp);
                if(src1 == src2) {
                    pc = operation->dst-1; // -1 because pc++ at end of loop
                }
            } break;
            case OP_CMP_EQ_BRANCH+1: case OP_CMP_EQ_BRANCH+2: case OP_CMP_EQ_BRANCH+3: case OP_CMP_EQ_BRANCH+4: {
                uint32_t sz=(op&7)==0?1:(8<<((op-1)&7));
                uint64_t src1 = operand_load(exe, sz, operation->flags_src1, operation->src1, sp);
                uint64_t src2 = operand_load(exe, sz, operation->flags_src2, operation->src2, sp);
                if(src1 == src2) {
                    pc = operation->dst-1; // -1 because pc++ at end of loop
                }
            } break;
            case OP_PLUS: case OP_MINUS: case OP_TIMES: case OP_DIVIDE: case OP_MODULUS: case OP_LT: case OP_LTE: {
                uint32_t sz = type_to_size(operation->type);
                uint64_t src1 = operand_load(exe, sz, operation->flags_src1, operation->src1, sp);
                uint64_t src2 = operand_load(exe, sz, operation->flags_src2, operation->src2, sp);
                uint64_t dst;
                switch(op) {
                    case OP_PLUS: dst = src1+src2; break;
                    case OP_MINUS: dst = src1-src2; break;
                    case OP_TIMES: dst = src1*src2; break;
                    case OP_LT: dst = (int64_t)src1<(int64_t)src2; break;
                    case OP_LTE: dst = (int64_t)src1<=(int64_t)src2; break;
                    case OP_DIVIDE: {
                        if(src2 == 0) {
                            fprintf(stderr, "fatal: division by zero in division\n");
                            exe->errtype=1;
                            return exe->errtype;
                        }
                        switch(operation->type) {
                            case TYPE_I8: dst=((int8_t)(src1))/(int8_t)src2; break;
                            case TYPE_U8: dst=((uint8_t)(src1))/(uint8_t)src2; break;
                            case TYPE_I16: dst=((int16_t)(src1))/(int16_t)src2; break;
                            case TYPE_U16: dst=((uint16_t)(src1))/(uint16_t)src2; break;
                            case TYPE_I32: dst=((int32_t)(src1))/(int32_t)src2; break;
                            case TYPE_U32: dst=((uint32_t)(src1))/(uint32_t)src2; break;
                            case TYPE_I64: dst=((int64_t)(src1))/(int64_t)src2; break;
                            case TYPE_U64: dst=((uint64_t)(src1))/(uint64_t)src2; break;
                            default: {
                                fprintf(stderr,"unknown type in division\n");
                                exe->errtype=1;
                                return exe->errtype;
                            }
                        }
                    } break;
                    case OP_MODULUS: {
                        if(src2 == 0) {
                            fprintf(stderr, "fatal: division by zero in modulus\n");
                            exit(EXIT_FAILURE);
                        }
                        switch(operation->type) {
                            case TYPE_I8: dst=((int8_t)(src1))%(int8_t)src2; break;
                            case TYPE_U8: dst=((uint8_t)(src1))%(uint8_t)src2; break;
                            case TYPE_I16: dst=((int16_t)(src1))%(int16_t)src2; break;
                            case TYPE_U16: dst=((uint16_t)(src1))%(uint16_t)src2; break;
                            case TYPE_I32: dst=((int32_t)(src1))%(int32_t)src2; break;
                            case TYPE_U32: dst=((uint32_t)(src1))%(uint32_t)src2; break;
                            case TYPE_I64: dst=((int64_t)(src1))%(int64_t)src2; break;
                            case TYPE_U64: dst=((uint64_t)(src1))%(uint64_t)src2; break;
                            default: {
                                fprintf(stderr,"unknown type in modulus\n");
                                exe->errtype=1;
                                return exe->errtype;
                            }
                        }
                    } break;
                }
                operand_store(exe, operation, dst, sz, sp);
            } break;
            default: {
                fprintf(stderr,"Illegal Instruction\n");
                exe->errtype=1;
                return exe->errtype;
            }
        }
        pc++;
    }
}

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
