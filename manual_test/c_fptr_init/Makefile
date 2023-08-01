LLVM_BIN=../../llvm-project/build/bin
CC=$(LLVM_BIN)/clang
CXX=$(LLVM_BIN)/clang++
OPT=$(LLVM_BIN)/opt

CFLAGS=-m64
CXXFLAGS=$(CFLAGS)
VIPFLAGS=-fsanitize=vip

.PHONY: all clean .FORCE

# simple testcases
testcases := raw_fptr.exe raw_fptr.ll
testcases += struct.exe struct.ll
testcases += 1darray.exe 1darray.ll
testcases += 2darray.exe 2darray.ll


# composite testcases
testcases += struct_1darray.exe struct_1darray.ll
testcases += struct_2darray.exe struct_2darray.ll
testcases += struct_struct.exe struct_struct.ll
testcases += 1darray_struct.exe 1darray_struct.ll
testcases += 2darray_struct.exe 2darray_struct.ll
testcases += struct4x.exe struct4x.ll

all: $(testcases)
%.exe: %.c .FORCE
	$(CC) $(VIPFLAGS) -o $@ $<
%.ll: %.c .FORCE
	$(CC) $(VIPFLAGS) -S -emit-llvm -o $@ $<
clean:
	rm $(wildcard *.exe) $(wildcard *.ll) $(wildcard *.bc) $(wildcard *.so)
