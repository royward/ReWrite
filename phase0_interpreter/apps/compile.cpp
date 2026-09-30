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
#include <stdexcept>
#include <sstream>

/*
./rewrite_cpp ../../../tests/clamp.rw "clamp(3,4,10)"
./rewrite_cpp ../../../tests/constants2.rw "hello()"
./rewrite_cpp ../../../tests/constants.rw "eval(3,4,5)"
./rewrite_cpp ../../../tests/equal.rw "equal(3,4)"
./rewrite_cpp ../../../tests/factorial.rw "fact(10)"
./rewrite_cpp ../../../tests/factorial2.rw "fact(10)"
./rewrite_cpp ../../../tests/factorial_errcheck.rw "fact(10)"
./rewrite_cpp ../../../tests/factorial_tail.rw "fact(10)"
./rewrite_cpp ../../../tests/fibonacci.rw "fib(10)"
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
./rewrite_cpp ../../../tests/swap.rw "swap(0,1)"
./rewrite_cpp ../../../tests/validate.rw "process(10)"
./rewrite_cpp ../../../tests/validate.rw "process(-10)"
*/

std::string load_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::stringstream msg;
        msg << "Failed to open file: " << path.string();
        throw std::runtime_error(msg.str());
    }
    const auto size = std::filesystem::file_size(path);
    std::string content(size, '\0');
    file.read(content.data(), static_cast<std::streamsize>(size));
    return content;
}

void write_string_to_file(const std::string& filename, const std::string& content) {
    std::ofstream out(std::filesystem::path(filename), std::ios::binary);
    if (!out) {
            throw std::runtime_error("could not open file for writing");
    }
    out.write(content.data(), content.size());
    if(!out.good()) {
        throw std::runtime_error("could not write to file");
    }
}

void write_binary_to_file(const std::string& filename, const uint8_t* data, size_t size) {
    std::ofstream out(std::filesystem::path(filename), std::ios::binary);
    if (!out) {
            throw std::runtime_error("could not open file for writing");
    }
    out.write(reinterpret_cast<const char*>(data), size);
    if(!out.good()) {
        throw std::runtime_error("could not write to file");
    }
}

int main(int argc, char** argv) {
    CLI::App app{"ReWrite Stage 1 interpreter"};
    bool fast = false;
    bool native = false;
    std::string infile;
    std::string outfile;
    app.add_option("infile", infile, "Source file to interpret")->required();
    app.add_flag("--fast", fast, "Enable fast execution mode");
    app.add_flag("--native", native, "Compile native");
    CLI11_PARSE(app, argc, argv);
    std::string popcodes="opcodes.rw";
    std::string pcompile="rewrite_compiler.rw";
    std::string pathin="/phase1/";
    try {
        std::string opcodes=load_file(RW_ROOT+pathin+popcodes);
        std::string compile=load_file(RW_ROOT+pathin+pcompile);
        Program prog(std::vector{std::make_pair(popcodes,opcodes),std::make_pair(pcompile,compile)},fast);
        std::string in=load_file(infile+".rw");
        std::vector<DataElement> args;
        DataElement::push_string(args,infile);
        DataElement::push_string(args,in);
        std::unordered_map<std::string, std::size_t> param_id_map;
        param_id_map["f0"]=0;
        param_id_map["s0"]=1;
        if(native) {
            std::cout << "native\n";
            std::vector<DataElement> results=prog.run_string_args(std::string("full_compile_native(f0,s0)"),param_id_map,args);
            write_string_to_file(infile+"_native.h",results[0].get_string());
            write_string_to_file(infile+"_native.ll",results[1].get_string());
        } else {
            std::vector<DataElement> results=prog.run_string_args(std::string("full_compile_vm(f0,s0)"),param_id_map,args);
            write_string_to_file(infile+".h",results[0].get_string());
            std::vector<uint64_t> data=results[1].get_vec_u64();
            write_binary_to_file(infile+".rwo",(const uint8_t*)data.data(),data.size()*8);
        }
        return 0;
    } catch(const std::runtime_error& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
}
