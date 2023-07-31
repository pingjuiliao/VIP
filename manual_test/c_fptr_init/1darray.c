#include <stdio.h>
#include <string.h>

int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mult(int a, int b) { return a * b; }
int divi(int a, int b) { return a / b; }

typedef int (*MathFunc)(int, int);
MathFunc arithmetics[4] = {
  add,
  sub,
  mult,
  divi,
};

void vuln(void) {
  strcpy((char *)&arithmetics,
         "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
}


int main(int argc, char** argv) {
  int a, b, c, d;
  a = (arithmetics[0])(1, 1);
  b = (arithmetics[1])(1, 1);
  c = (arithmetics[2])(1, 1);
  d = (arithmetics[3])(1, 1);
  printf("add: %8d\nsub: %8d\nmul: %8d\ndiv: %8d\n", a, b, c, d);
  vuln();
  a = (arithmetics[0])(1337, 256);
  b = (arithmetics[1])(1337, 256);
  c = (arithmetics[2])(1337, 256);
  d = (arithmetics[3])(1337, 256);
  printf("add: %8d\nsub: %8d\nmul: %8d\ndiv: %8d\n", a, b, c, d);
  return 0;
}
