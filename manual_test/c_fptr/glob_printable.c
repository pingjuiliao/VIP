#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct _Printable {
  char buffer[8];
  int (*print_func)(const char*);
} Printable;

Printable ptb = {"name", puts};

void test_glob_bufovfl(void) {
  int r = 0;
  printf("ptb: %p\n", (void *)&ptb);
  printf(" printf_func b4: %p\n", ptb.print_func);

  strcpy((char *)&ptb.buffer, "12345678abcdefg");
  printf(" printf_func after: %p\n", ptb.print_func);
  ptb.print_func((const char*)&ptb.buffer);
}

/*void __attribute__((constructor)) printable_init(void) {
  puts("constructing");
}

void __attribute__((constructor)) printable_init2(void) {
  puts("constructing2");
}*/

int main(int argc, char** argv) {
  test_glob_bufovfl();
  puts("Bye!");
  return 0;
}



