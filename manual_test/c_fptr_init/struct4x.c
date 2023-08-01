#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _Printable {
  char buffer[8];
  int (*print_func)(const char*);
} Printable;

typedef struct _MetaPrintable {
  int id;
  Printable ptb;
} MetaPrintable;

typedef struct _SuperPrintable {
  char name[0x200];
  MetaPrintable mtpb;
} SuperPrintable;

typedef struct _OmegaPrintable {
  char owner[0x200];
  int ref_count;
  SuperPrintable sptb;
} OmegaPrintable;

OmegaPrintable optb = {"hello", 2, {"pingjui", {1337, {"hello", puts}}}};

void test_glob_bufovfl(void) {
  printf("optb.sptb.mtpb: %p\n", (void *)&optb.sptb.mtpb);
  printf(" printf_func b4: %p\n", optb.sptb.mtpb.ptb.print_func);

  strcpy((char *)&optb.sptb.mtpb.ptb.buffer, "12345678abcdefg");
  printf(" printf_func after: %p\n", optb.sptb.mtpb.ptb.print_func);
  optb.sptb.mtpb.ptb.print_func((const char*)&optb.sptb.mtpb.ptb.buffer);
}

int main(int argc, char** argv) {
  test_glob_bufovfl();
  puts("Bye!");
  return 0;
}



