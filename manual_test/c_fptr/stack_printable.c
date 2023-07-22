#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CPI_ERROR 0xcafe

typedef struct _Printable {
  char buffer[8];
  int (*print_func)(const char*);
} Printable;

void test_stack_bufovfl(void) {
  int r = 0;
  Printable ptb;
  ptb.print_func = puts;
  printf(" printf_func b4: %p\n", ptb.print_func);

  strcpy((char *)&ptb.buffer, "12345678abcdefgh12345678abcdefgh");
  printf(" printf_func after: %p\n", ptb.print_func);

  ptb.print_func((const char *)&ptb.buffer);
}

int main(int argc, char** argv) {
  test_stack_bufovfl();
  return 0;
}



