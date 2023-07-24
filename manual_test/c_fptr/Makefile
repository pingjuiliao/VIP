LLVM_BIN=../../llvm-project/build/bin
CC=$(LLVM_BIN)/clang
CXX=$(LLVM_BIN)/clang++
OPT=$(LLVM_BIN)/opt

CFLAGS=-m64
CXXFLAGS=$(CFLAGS)
VIPFLAGS=-fsanitize=vip

.PHONY: all clean libCPI

testcases := heap_printable.exe heap_printable.ll
testcases += stack_printable.exe stack_printable.ll

all: libCPI $(testcases)
%.exe: %.c
	$(CC) $(VIPFLAGS) -o $@ $<
%.ll: %.c
	$(CC) $(VIPFLAGS) -S -emit-llvm -o $@ $<
clean:
	rm $(wildcard *.exe) $(wildcard *.ll) $(wildcard *.bc) $(wildcard *.so)
