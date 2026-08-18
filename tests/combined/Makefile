# Define tools and paths
REWRITE_CPP = ../../build/release/phase0_interpreter/rewrite_cpp
COMPILER_RW = ../../phase1/rewrite_compiler.rw
CORE_DEPS = tests.h combined.rw $(REWRITE_CPP) $(COMPILER_RW)

# Phony targets (they don't represent actual files on disk)
.PHONY: test_vm test_native clean

# 1. VM Test Pipeline
test_vm: combined.exe
	./combined.exe

# Only recompile/re-run if the C source files change
combined.exe: ../../phase1_vm/src/vm.c main.c $(CORE_DEPS)
	$(REWRITE_CPP) $(COMPILER_RW) 'compile("combined.rw","combined","vm")'
	clang -ggdb3 -I../../phase1_vm/include/ ../../phase1_vm/src/vm.c main.c -o $@

# 2. Native Test Pipeline
test_native: combined_native.exe
	./combined_native.exe

# Only re-compile if C files or the generated .ll file change
combined_native.exe: ../../native/src/rewrite_base.c main_native.c $(CORE_DEPS)
	$(REWRITE_CPP) $(COMPILER_RW) 'compile("combined.rw","combined_native","x86-64")'
	clang -ggdb3 -I../../native/include/ ../../native/src/rewrite_base.c main_native.c combined_native.ll -o $@

clean:
	rm -f combined.exe combined_native.exe combined_native.ll combined.rwo
