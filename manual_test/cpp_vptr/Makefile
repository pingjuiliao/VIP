LLVM_BIN=../../llvm-project/build/bin
CC=$(LLVM_BIN)/clang
CXX=$(LLVM_BIN)/clang++

CFLAGS=-m64
CXXFLAGS=$(CFLAGS)
VIPFLAGS=-fsanitize=vip

# tests
tests := employees_vip.exe employees.ll employees_vuln.exe
tests += account_vip.exe account_vuln.exe
tests += tomato_vip.exe
.PHONY: clean

all: $(hellos) $(tests)
%.ll: %.cc
	$(CXX) $(CXXFLAGS) -S -emit-llvm -o $@ $<
%_vuln.exe: %.cc
	$(CXX) $(CXXFLAGS) -o $@ $<
%_vip.exe: %.cc
	$(CXX) $(CXXFLAGS) $(VIPFLAGS) -o $@ $<
clean:
	rm $(wildcard *.exe) $(wildcard *.ll)
