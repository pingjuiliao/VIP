#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct _Printable {
  char buffer[8];
  int (*print_func)(const char*);
} Printable;

void test_heap_bufovfl(void) {
  int r = 0;
  Printable *ptb = (Printable *) malloc(sizeof(struct _Printable));
  printf("ptb: %p\n", (void *)ptb);
  ptb->print_func = puts;
  printf(" printf_func b4: %p\n", ptb->print_func);

  strcpy((char *)&ptb->buffer, "12345678abcdefg");
  printf(" printf_func after: %p\n", ptb->print_func);
  ptb->print_func((const char*)&ptb->buffer);
}

int main(int argc, char** argv) {
  test_heap_bufovfl();
  puts("Bye!");
  return 0;
}



