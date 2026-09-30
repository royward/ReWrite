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

extern "C" {
#include "vm.h"
}
#include <CLI/CLI.hpp>
#include <fstream>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <format>
#include <print>
#include <cinttypes>

int main(int argc, char** argv) {
    CLI::App app{"ReWrite VM disassembler"};
    std::string filename;
    app.add_option("file", filename, "Source file to interpret")->required();
    CLI11_PARSE(app, argc, argv);
    Program p;
    program_load(&p,filename.c_str());
    program_disassemble(&p,stdout);
    program_unload(&p);
    return EXIT_SUCCESS;
}
