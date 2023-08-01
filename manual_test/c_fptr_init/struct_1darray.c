#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*PRINT_FUNC)(const char*);


typedef struct _Printable {
  char buffer[8];
  PRINT_FUNC print_func_list[2];
} Printable;

int my_puts(const char* s) {
  return printf("[MY PUTS] %s\n", s);
}

Printable ptb = {"name", {puts, my_puts}};

void test_glob_bufovfl(void) {
  int r = 0;
  printf("ptb: %p\n", (void *)&ptb);
  printf(" printf_func b4: %p\n", ptb.print_func_list[1]);

  strcpy((char *)&ptb.buffer, "12345678abcdefgh12345678abcdefg");
  printf(" printf_func after: %p\n", ptb.print_func_list[1]);
  (ptb.print_func_list[1])((const char*)&ptb.buffer);
}


int main(int argc, char** argv) {
  test_glob_bufovfl();
  puts("Bye!");
  return 0;
}



