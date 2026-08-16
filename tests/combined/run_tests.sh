set -x
../../build/release/phase0_interpreter/rewrite_cpp ../../phase1/rewrite_compiler.rw 'compile("combined.rw","combined","vm")'
clang -O2 -I../../phase1_vm/include/ ../../phase1_vm/src/vm.c main.c -o combined.exe
./combined.exe
set +x
