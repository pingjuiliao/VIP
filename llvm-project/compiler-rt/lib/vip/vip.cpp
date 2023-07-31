#include "vip.h"

#include <asm/prctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

void __attribute__((constructor)) vip_init(void) {
  int r = 0;
  void* safe_region = mmap(NULL, kTopLevelMappedSize,
                           PROT_READ|PROT_WRITE,
                           MAP_ANONYMOUS|MAP_PRIVATE,
                           -1, 0);
  
  if (!safe_region) {
    fprintf(stderr, VIP_ERROR "cannot map safe region\n");
    exit(-1);
  }
  
  memset(safe_region, 0, kTopLevelMappedSize);
  r = syscall(SYS_arch_prctl, ARCH_SET_GS, safe_region);
  if (r != 0) {
    fprintf(stderr, VIP_ERROR "cannot set %%gsbase\n");
    exit(-1);
  }
  safe_region = NULL;
#ifndef VIP_NO_VERBOSE
  puts(VIP_INIT "Safe region init success");
#endif
  InitializedPointerNode* node = vip_init_ptr_list;
  InitializedPointerNode* tmp;
  while (node) {
    vip_write64(node->ptr);
    tmp = node->next;
    free(node);
    node = tmp;
  }
  vip_init_ptr_list = vip_init_ptr_list_end = 0;
}

void vip_pending_write64(void** ptr) {
  InitializedPointerNode *ptr_node;
  ptr_node = (InitializedPointerNode *) malloc(sizeof(InitializedPointerNode));
  ptr_node->ptr = ptr;
  ptr_node->next = NULL;
  if (vip_init_ptr_list_end) {
    vip_init_ptr_list_end->next = ptr_node; 
  } else {
    vip_init_ptr_list = ptr_node;
  }
  vip_init_ptr_list_end = ptr_node;
}

void vip_write64(void** ptr) {
  size_t top_idx;
  size_t bot_offset;
  void* entry;

#ifndef VIP_NO_VERBOSE
  fprintf(stderr, VIP_WRITE "safe[%p] := %p\n", ptr, *ptr);
#endif

  asm volatile(
    "movq %[ptr], %[top_idx];"
    "shrq $26, %[top_idx];"
    "movq %[ptr], %[bot_offset];"
    "andq $0x3fffff8, %[bot_offset];"
    : [top_idx]"=m"(top_idx),
      [bot_offset]"=m"(bot_offset)
    : [ptr] "r"(ptr)
  );
  asm volatile(
    "movq %%gs:(, %[top_idx], 8), %[entry]"
    : [entry] "=r"(entry)
    : [top_idx] "r"(top_idx)
  );

  if (!entry) {
    entry = mmap(NULL, kBotLevelMappedSize,
                 PROT_READ|PROT_WRITE,
                 MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    asm volatile(
      "movq %[entry], %%gs:(,%[top_idx], 8);"
      :
      : [entry] "r"(entry),
        [top_idx] "r"(top_idx)
    );
  }
  asm volatile(
    "movq (%[ptr]), %%r10;"
    "movq %%r10, (%[entry], %[bot_offset]);"
    : 
    : [ptr] "r"(ptr),
      [entry] "r"(entry),
      [bot_offset] "r"(bot_offset)
    : "r10"
  );

  entry = NULL;
}

void vip_assert(void** ptr) {
  void** safe_copy;
  void* entry;
#ifndef VIP_NO_VERBOSE
  fprintf(stderr, VIP_ASSERT "safe[%p] == %p ??\n", ptr, *ptr);
#endif
  asm volatile(
    "movq %[ptr], %[entry];"
    "shrq $26, %[entry];"
    "movq %%gs:(, %[entry], 8), %[entry];"
    : [entry] "=r"(entry)
    : [ptr] "r"(ptr)
  );

  if (!entry) {
    fprintf(stderr, VIP_VIOLATION "check failed on %p\n", ptr);
    fprintf(stderr, " No vip superpage entry found\n");
    exit(-1);
  }

  asm volatile(
    "movq %[ptr], %%r11;"
    "andq $0x3fffff8, %%r11;"
    "movq (%[entry], %%r11), %[safe_copy];"
    : [safe_copy]"=r"(safe_copy)
    : [ptr] "r"(ptr),
      [entry] "r"(entry) 
    : "r11"
  );

  if (safe_copy != *ptr) {
    fprintf(stderr, VIP_VIOLATION "check failed on %p\n", ptr);
    fprintf(stderr, " safe_copy (before): %p\n", safe_copy);
    fprintf(stderr, " vuln pointer (now): %p\n", *ptr);
    exit(-1);
  }
  entry = NULL;
}

void vip_assert_debug(void** ptr, char* caller) {
  char buffer[0x200];
  void** safe_copy;
  void* entry;
  fprintf(stderr, VIP_DEBUG "Caller: %s\n", caller);

  if (vip_init_ptr_list) {
    fprintf(stderr, VIP_ASSERT "init list is not null\n");
    exit(-1);
  }
#ifndef VIP_NO_VERBOSE
  fprintf(stderr, VIP_ASSERT "safe[%p] == %p ??\n", ptr, *ptr);
#endif
  asm volatile(
    "movq %[ptr], %[entry];"
    "shrq $26, %[entry];"
    "movq %%gs:(, %[entry], 8), %[entry];"
    : [entry] "=r"(entry)
    : [ptr] "r"(ptr)
  );

  if (!entry) {
    fprintf(stderr, VIP_VIOLATION "check failed on %p\n", ptr);
    fprintf(stderr, " No vip superpage entry found\n");
    FILE* fp = fopen("/proc/self/maps", "rb");
    memset(buffer, 0, sizeof(buffer));
    while (fgets(buffer, sizeof(buffer), fp)) {
      fputs(buffer, stderr);
    }
  }

  asm volatile(
    "movq %[ptr], %%r11;"
    "andq $0x3fffff8, %%r11;"
    "movq (%[entry], %%r11), %[safe_copy];"
    : [safe_copy]"=r"(safe_copy)
    : [ptr] "r"(ptr),
      [entry] "r"(entry)
    : "r11"
  );

  if (safe_copy != *ptr) {
    fprintf(stderr, VIP_VIOLATION "check failed on %p\n", ptr);
    fprintf(stderr, " safe_copy (before): %p\n", safe_copy);
    fprintf(stderr, " vuln pointer (now): %p\n", *ptr);
    FILE* fp = fopen("/proc/self/maps", "rb");
    memset(buffer, 0, sizeof(buffer));
    while (fgets(buffer, sizeof(buffer), fp)) {
      fputs(buffer, stderr);
    }
    exit(-1);
  }
  entry = NULL;
}
