# Define tools and paths
REWRITE_CPP = ../../build-release/phase0_interpreter/rw_compile
COMPILER_RW = ../../phase1/rewrite_compiler.rw

# Phony targets (they don't represent actual files on disk)
.PHONY: test_vm test_native clean

FORCE:

test_vm: combined.exe
	./combined.exe

combined.exe: FORCE
	$(REWRITE_CPP) --fast combined
	clang -ggdb3 -std=c11 -pedantic -Wall -Wextra -I../../vm/include/ -I../../vm/generated/ ../../vm/src/vm.c main.c -o $@

test_native: combined_native.exe
	./combined_native.exe

combined_native.exe: FORCE
	$(REWRITE_CPP) --fast --native combined
	opt-19 -passes=verify -disable-output combined_native.ll
	clang -O3 -S combined_native.ll
	clang -O3 -std=c11 -pedantic -Wall -Wextra -I../../native/include/ ../../native/src/rewrite_base.c main_native.c combined_native.ll -o $@

clean:
	rm -f combined.exe combined_native.exe combined_native.ll combined_native.s combined.rwo
