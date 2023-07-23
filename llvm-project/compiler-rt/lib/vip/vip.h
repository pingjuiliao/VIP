#ifndef VALUE_INVARIANT_PROPERTY_H
#define VALUE_INVARIANT_PROPERTY_H
#include <stdlib.h>

// Banners
#define VIP_ERROR "[VIP_ERROR]     "
#define VIP_VIOLATION "[VIP_VIOLATION] "
#define VIP_INIT "[VIP_INIT]      "
#define VIP_WRITE "[VIP_WRITE]     "
#define VIP_ASSERT "[VIP_ASSERT]    "

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

#ifdef __cplusplus
extern "C" {
#endif

void __attribute__((constructor)) vip_init(void);
void vip_write64(void**);
void vip_assert(void**);

#ifdef __cplusplus
}
#endif
#endif  // VALUE_INVARIANT_PROPERTY_H

