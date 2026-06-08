/**
 * @file mem_pool.h
 * @brief 固定大小堆内存池，为 newlib 提供 malloc/free/realloc 覆盖。
 *
 * 嵌入式无 MMU 时常用静态池替代 sbrk 堆增长；mem_pool.c 重载 malloc 族并
 * 令 _sbrk_r 失败，迫使所有堆分配落在 MEM_POOL_SIZE（默认 2048）内。
 * 编译前可在工程宏中重定义 MEM_POOL_SIZE。
 */

#ifndef MEM_POOL_H
#define MEM_POOL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MEM_POOL_SIZE
/** @brief 静态池总字节数；过小会导致 cJSON/printf 分配失败。 */
#define MEM_POOL_SIZE 2048U
#endif

/** @brief 将池初始化为单块空闲区；也可依赖首次 malloc 惰性初始化。 */
void mem_pool_init(void);

/** @brief 池容量（字节）。 */
size_t mem_pool_total(void);

/** @brief 已分配块占用总和（含每块头 mem_block_t）。 */
size_t mem_pool_used(void);

/** @brief 空闲块占用总和（含块头）。 */
size_t mem_pool_free_bytes(void);

/** @brief 最大连续可分配载荷（不含块头），用于评估大块分配是否可能成功。 */
size_t mem_pool_largest_free(void);

#ifdef __cplusplus
}
#endif

#endif
