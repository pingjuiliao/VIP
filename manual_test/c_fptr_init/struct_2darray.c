#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*PRINT_FUNC)(const char*);

const char* cyclic = "12345678abcdefgh12345678abcdefgh"
                     "12345678abcdefgh12345678abcdefgh"
                     "12345678abcdefgh12345678abcdefgh"
                     "12345678abcdefgh12345678abcdefgh"
                     "12345678abcdefgh12345678abcdefgh"
                     "12345678abcdefgh12345678abcdefgh"
                     "12345678abcdefgh12345678abcdefgh"
                     "12345678abcdefgh12345678abcdefgh"
                     "12345678abcdefgh12345678abcdefg";

typedef struct _Printable {
  char buffer[8];
  PRINT_FUNC print_func_list[2][3];
} Printable;

int foo_puts(const char* s) {
  return printf("[FOO] %s\n", s);
}

int my_puts(const char* s) {
  return printf("[MY PUTS] %s\n", s);
}

Printable ptb = {"name", 
                 {{puts, my_puts, foo_puts}, 
                  {my_puts, foo_puts, puts}}};

void test_glob_bufovfl(void) {
  printf("ptb: %p\n", (void *)&ptb);
  printf(" printf_func b4: %p\n", ptb.print_func_list[1][1]);

  strcpy((char *)&ptb.buffer, cyclic);
  printf(" printf_func after: %p\n", ptb.print_func_list[1][1]);
  (ptb.print_func_list[1][1])((const char*)&ptb.buffer);
}


int main(int argc, char** argv) {
  test_glob_bufovfl();
  puts("Bye!");
  return 0;
}



