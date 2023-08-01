#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct _Printable {
  char buffer[8];
  int (*print_func)(const char*);
} Printable;

Printable ptb[3] = {
  {"name", puts},
  {"ping", puts},
  {"jui", puts}
};

void test_glob_bufovfl(void) {
  printf("ptb: %p\n", (void *)&ptb);
  printf(" printf_func b4: %p\n", ptb[1].print_func);

  strcpy((char *)&ptb[1].buffer, "12345678abcdefg");
  printf(" printf_func after: %p\n", ptb[1].print_func);
  ptb[1].print_func((const char*)&ptb[1].buffer);
}

int main(int argc, char** argv) {
  test_glob_bufovfl();
  puts("Bye!");
  return 0;
}



