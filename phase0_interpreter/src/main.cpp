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

#include "program.hpp"
#include <CLI/CLI.hpp>
#include <fstream>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <format>
#include <print>

/*
./rewrite_cpp ../../../tests/clamp.rw "clamp(3,4,10)"
./rewrite_cpp ../../../tests/constants2.rw "hello()"
./rewrite_cpp ../../../tests/constants.rw "eval(3,4,5)"
./rewrite_cpp ../../../tests/clamp.rw "clamp(3,4,10)"
./rewrite_cpp ../../../tests/constants2.rw "hello()"
./rewrite_cpp ../../../tests/constants.rw "eval(3,4,5)"
./rewrite_cpp ../../../tests/equal.rw "equal(3,4)"
./rewrite_cpp ../../../tests/factorial.rw "fact(10)"
./rewrite_cpp ../../../tests/factorial2.rw "fact(10)"
./rewrite_cpp ../../../tests/factorial_errcheck.rw "fact(10)"
./rewrite_cpp ../../../tests/listn2.rw "listn2(10)"
./rewrite_cpp ../../../tests/listn.rw "listn(10)"
./rewrite_cpp ../../../tests/member.rw "member(2,{1,2,3})"
./rewrite_cpp ../../../tests/min_max.rw "sub(min_max(4,10))"
./rewrite_cpp ../../../tests/misc_splat.rw "middle({1,2,3,4})"
./rewrite_cpp ../../../tests/misc_splat.rw "flatten({{1,2},{3}})"
./rewrite_cpp ../../../tests/nprime.rw "nprime(101)"
./rewrite_cpp ../../../tests/nqueens_bitmask.rw "nqueens(8)"
./rewrite_cpp ../../../tests/nqueens.rw "nqueens(8)"
./rewrite_cpp ../../../tests/post_arrow_match.rw "use_twice(3)"
./rewrite_cpp ../../../tests/reverse.rw "reverse({1,2,3,4})"
./rewrite_cpp ../../../tests/roman_numerals.rw "int_to_roman(1995)"
./rewrite_cpp ../../../tests/roman_numerals.rw 'roman_to_int("mcmxcv")'
./rewrite_cpp ../../../tests/string_to_int.rw 'string_to_int("-256")'
*/

std::string load_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open file: {}", path.string()));
    }
    const auto size = std::filesystem::file_size(path);
    std::string content(size, '\0');
    file.read(content.data(), static_cast<std::streamsize>(size));
    return content;
}

int main(int argc, char** argv) {
    CLI::App app{"ReWrite Stage 1 interpreter"};
    std::string filename;
    std::string callexpr;
    app.add_option("file", filename, "Source file to interpret")->required();
    app.add_option("call", callexpr, "Expression to evaluate")->required();
    CLI11_PARSE(app, argc, argv);
    try {
        std::string s=load_file(filename);
        //std::println("{}",s);
        Program prog(s);
        std::vector<DataElement> results=prog.run_string(callexpr);
        std::println("Results:");
        for(auto& r : results) {
            println("{}",r.to_string());
        }
    } catch(const std::runtime_error& e) {
        std::println("Error: {}", e.what());
    }
}
