#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct _Printable {
  char buffer[8];
  int (*print_func)(const char*);
} Printable;

int leet_puts(const char* s) {
  return printf("As a leet, I present %s\n", s);
}
Printable ptb[3][2] = {
  {{"name", puts}, {"space", puts}},
  {{"ping", puts}, {"jui", puts}},
  {{"leet", leet_puts}, {"code", leet_puts}}
};

void test_glob_bufovfl(void) {
  printf("ptb: %p\n", (void *)&ptb);
  printf(" printf_func b4: %p\n", ptb[2][1].print_func);

  strcpy((char *)&ptb[2][1].buffer, "12345678abcdefg");
  printf(" printf_func after: %p\n", ptb[2][1].print_func);
  ptb[2][1].print_func((const char*)&ptb[2][1].buffer);
}

int main(int argc, char** argv) {
  test_glob_bufovfl();
  puts("Bye!");
  return 0;
}



