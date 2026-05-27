#ifndef MEM_POOL_H
#define MEM_POOL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MEM_POOL_SIZE
#define MEM_POOL_SIZE 2048U
#endif

void mem_pool_init(void);
size_t mem_pool_total(void);
size_t mem_pool_used(void);
size_t mem_pool_free_bytes(void);
size_t mem_pool_largest_free(void);

#ifdef __cplusplus
}
#endif

#endif
