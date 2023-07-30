#ifndef VALUE_INVARIANT_PROPERTY_H
#define VALUE_INVARIANT_PROPERTY_H
#include <stdlib.h>

#define VIP_NO_VERBOSE

// Banners
#define VIP_ERROR "[VIP_ERROR]     "
#define VIP_VIOLATION "[VIP_VIOLATION] "
#define VIP_INIT "[VIP_INIT]      "
#define VIP_WRITE "[VIP_WRITE]     "
#define VIP_ASSERT "[VIP_ASSERT]    "
#define VIP_DEBUG "[VIP_DEBUG]     "

/*
 *               --22--      --23--  
 *   63       47       26 25        3 2   0  
 *  +--------+-----------+-----------+----+
 *  | Unused | top-level | bot-level |    |
 *  +--------+-----------+-----------+----+
 **/

const int kNumTopLevelEntry = (1 << 22);
const size_t kTopLevelMappedSize = kNumTopLevelEntry * sizeof(size_t);
const int kNumBotLevelEntry = (1 << 23);
const size_t kBotLevelMappedSize = kNumBotLevelEntry * sizeof(size_t);

struct InitializedPointerNode {
  void** ptr;
  InitializedPointerNode *next;
};

InitializedPointerNode* vip_init_ptr_list = NULL;
InitializedPointerNode* vip_init_ptr_list_end = NULL;

#ifdef __cplusplus
extern "C" {
#endif

void __attribute__((constructor)) vip_init(void);
void vip_write64(void**);
void vip_pending_write64(void**);
void vip_assert(void**);
void vip_assert_debug(void**, char*);

#ifdef __cplusplus
}
#endif
#endif  // VALUE_INVARIANT_PROPERTY_H

