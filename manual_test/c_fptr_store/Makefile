LLVM_BIN=../../llvm-project/build/bin
CC=$(LLVM_BIN)/clang
CXX=$(LLVM_BIN)/clang++
OPT=$(LLVM_BIN)/opt

CFLAGS=-m64
CXXFLAGS=$(CFLAGS)
VIPFLAGS=-fsanitize=vip

.PHONY: all clean .FORCE

testcases := heap_printable.exe heap_printable.ll
testcases += stack_printable.exe stack_printable.ll
testcases += glob_printable.exe glob_printable.ll
testcases += glob_fptr.exe glob_fptr.ll

all: $(testcases)
%.exe: %.c .FORCE
	$(CC) $(VIPFLAGS) -o $@ $<
%.ll: %.c .FORCE
	$(CC) $(VIPFLAGS) -S -emit-llvm -o $@ $<
clean:
	rm $(wildcard *.exe) $(wildcard *.ll) $(wildcard *.bc) $(wildcard *.so)
