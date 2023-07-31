#include <stdio.h>
#include <string.h>
int (*fptr)(const char*) = puts;

void vuln() {
  strcpy((char *)&fptr, "hewok");
}

int main(int argc, char** argv) {
  vuln();
  (*fptr)("hello world");
  return 0;
}
