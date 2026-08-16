set -x
../../build/release/phase0_interpreter/rewrite_cpp ../../phase1/rewrite_compiler.rw 'compile("combined.rw","combined_native","x86-64")'
clang -O2 -ggdb3 -I../../native/include/ ../../native/src/rewrite_base.c main_native.c combined_native.ll -o combined_native.exe
./combined_native.exe
set +x
