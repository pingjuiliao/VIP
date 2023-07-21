#include <stdio.h>
#include <string.h>
#include "vip.h"

typedef struct _Overflowable {
  char buf[0x10];
  int (*fptr)(const char*);
} Overflowable;


int main(int argc, char** argv) {
  Overflowable ovfl;
  ovfl.fptr = puts; 
  vip_write64((void **) &ovfl.fptr);
  strcpy(ovfl.buf, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    
  vip_assert((void **) &ovfl.fptr);
  (*ovfl.fptr)((const char *)ovfl.buf);
  return 0;
}
