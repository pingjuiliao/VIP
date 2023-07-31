#include <stdio.h>
#include <string.h>
int (*fptr)(const char*);

void vuln() {
  strcpy((char *)&fptr, "AAAAAAAAAA");
}

int main(int argc, char** argv) {
  fptr = puts;
  vuln();
  (*fptr)("hello world");
  return 0;
}
