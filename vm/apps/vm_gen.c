#include "vm.h"
#include "../../build/generated/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int write_file(const char* path, const char* text) {
    FILE* f = fopen(path, "wb");
    if (!f) { perror(path); return -1; }
    fputs(text, f);
    return fclose(f);
}

static int write_output(const char* dir, const char* name, char* text) {
    char path[4096];
    if (snprintf(path, sizeof path, "%s/%s", dir, name) >= (int)sizeof path) {
        fprintf(stderr, "path too long: %s/%s\n", dir, name);
        return -1;
    }
    int err=write_file(path, text);
    free(text);
    return err;
}

int main(int argc, char** argv) {
    char path[4096];
    const char* outdir = ".";
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0)) {
            if (i + 1 >= argc) {
                fprintf(stderr, "%s requires a directory\n", argv[i]);
                return EXIT_FAILURE;
            }
            outdir = argv[++i];
            continue;
        }
    }
    snprintf(path, sizeof path, "%s/%s", RW_ROOT, "build/generated/vm.rwo");
    Program p;
    program_load(&p, path);
    RWInstance* exe=(RWInstance*)malloc(sizeof(RWInstance));
    rw_instance_init(exe,&p);
    char* defines;
    char* oplist;
    char* disassemble;
    char* execute;
    char* oplabels;
    int ret_code=rw_generate_inc_files(exe,&defines,&oplist,&disassemble,&execute,&oplabels);
    write_output(outdir,"defines.inc",defines);
    write_output(outdir,"oplist.inc",oplist);
    write_output(outdir,"disassemble.inc",disassemble);
    write_output(outdir,"execute.inc",execute);
    write_output(outdir,"oplabels.inc",oplabels);
    rw_instance_unload(exe);
    program_unload(&p);
    return EXIT_SUCCESS;
}
