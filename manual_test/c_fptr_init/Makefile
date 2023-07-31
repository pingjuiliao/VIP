LLVM_BIN=../../llvm-project/build/bin
CC=$(LLVM_BIN)/clang
CXX=$(LLVM_BIN)/clang++
OPT=$(LLVM_BIN)/opt

CFLAGS=-m64
CXXFLAGS=$(CFLAGS)
VIPFLAGS=-fsanitize=vip

.PHONY: all clean .FORCE

testcases := raw_fptr.exe raw_fptr.ll
testcases += struct.exe struct.ll
testcases += 1darray.exe 1darray.ll


all: $(testcases)
%.exe: %.c .FORCE
	$(CC) $(VIPFLAGS) -o $@ $<
%.ll: %.c .FORCE
	$(CC) $(VIPFLAGS) -S -emit-llvm -o $@ $<
clean:
	rm $(wildcard *.exe) $(wildcard *.ll) $(wildcard *.bc) $(wildcard *.so)
