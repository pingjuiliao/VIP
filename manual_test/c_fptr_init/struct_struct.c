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

MetaPrintable mptb = {1337, {"name", puts}};

void test_glob_bufovfl(void) {
  printf("mptb: %p\n", (void *)&mptb);
  printf(" printf_func b4: %p\n", mptb.ptb.print_func);

  strcpy((char *)&mptb.ptb.buffer, "12345678abcdefg");
  printf(" printf_func after: %p\n", mptb.ptb.print_func);
  mptb.ptb.print_func((const char*)&mptb.ptb.buffer);
}

int main(int argc, char** argv) {
  test_glob_bufovfl();
  puts("Bye!");
  return 0;
}



