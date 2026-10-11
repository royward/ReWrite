// ============================================================================
// File: main.cpp
// Description: Command line wrapper
// ============================================================================
// Copyright 2026 Roy Ward
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(FILE* out, const char* prog) {
    fprintf(out,
        "ReWrite VM disassembler\n"
        "Usage: %s [OPTIONS] file\n"
        "\n"
        "Positionals:\n"
        "  file        REQUIRED Source file to interpret\n"
        "\n"
        "Options:\n"
        "  -h,--help   Print this help message and exit\n",
        prog);
}

int main(int argc, char** argv) {
    const char* filename = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;
        }
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "The following argument was not expected: %s\n", argv[i]);
            fprintf(stderr, "Run with --help for more information.\n");
            return EXIT_FAILURE;
        }
        if (filename != NULL) {
            fprintf(stderr, "The following argument was not expected: %s\n", argv[i]);
            fprintf(stderr, "Run with --help for more information.\n");
            return EXIT_FAILURE;
        }
        filename = argv[i];
    }

    if (filename == NULL) {
        fprintf(stderr, "file is required\n");
        fprintf(stderr, "Run with --help for more information.\n");
        return EXIT_FAILURE;
    }

    Program p;
    program_load(&p, filename);
    program_disassemble(&p, stdout);
    program_unload(&p);
    return EXIT_SUCCESS;
}
