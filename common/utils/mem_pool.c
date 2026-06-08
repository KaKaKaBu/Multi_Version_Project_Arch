/**
 * @file mem_pool.c
 * @brief 首次适配（first-fit）固定内存池及 newlib malloc 族重载。
 *
 * ## 块布局
 *
 * 池为连续 uint8_t[N]，逻辑上切分为 [mem_block_t 头 | 载荷 ...] 链表。
 * 头中 size 为载荷字节数，used 标记分配/空闲；遍历靠 mem_block_span = 头对齐大小 + size。
 *
 * ## 算法
 *
 * - malloc：首次适配空闲块，必要时 mem_split_block 切出尾部剩余空闲。
 * - free：标记空闲后 mem_coalesce_all 向前合并相邻空闲块。
 * - realloc：缩小则 split；向后邻块空闲则扩展；否则 malloc+memcpy+free。
 *
 * ## newlib 集成
 *
 * 提供 malloc/free/realloc/calloc 与 _malloc_r 等；_sbrk_r 恒失败，禁止堆向 RAM 尾增长。
 */

#include "mem_pool.h"
#include <reent.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/** @brief 分配对齐字节数。 */
#define MEM_ALIGN 4U
/** @brief 可分配块的最小载荷字节数。 */
#define MEM_MIN_BLOCK 8U

/** @brief 池中每个已分配或空闲块前的头部。 */
typedef struct mem_block {
    size_t size;   ///< 载荷字节数（不含头部）。
    uint8_t used;  ///< 非零表示块已分配。
} mem_block_t;

static uint8_t mem_pool_storage[MEM_POOL_SIZE] __attribute__((aligned(MEM_ALIGN), used)); ///< 全部池块的静态存储区。
static uint8_t mem_pool_ready __attribute__((used)); ///< mem_pool_init 或首次分配后为非零。

/** @brief 返回 mem_block_t 头部的对齐字节大小。 */
static size_t mem_block_header_size(void)
{
    return (sizeof(mem_block_t) + (MEM_ALIGN - 1U)) & ~(MEM_ALIGN - 1U);
}

/** @brief 将大小向上对齐到 MEM_ALIGN 边界。 */
static size_t mem_align_up(size_t value)
{
    return (value + (MEM_ALIGN - 1U)) & ~(MEM_ALIGN - 1U);
}

/**
 * @brief 返回池中指定字节偏移处的块头部。
 * @param[in] offset 相对 mem_pool_storage 起始的字节偏移。
 * @return 该偏移处的块头部指针。
 */
static mem_block_t *mem_block_at(size_t offset)
{
    return (mem_block_t *)(void *)(mem_pool_storage + offset);
}

/** @brief 由已分配块头部得到用户载荷指针。 */
static void *mem_block_payload(mem_block_t *block)
{
    return (uint8_t *)(void *)block + mem_block_header_size();
}

/** @brief 由用户载荷指针反推块头部。 */
static mem_block_t *mem_block_from_payload(void *ptr)
{
    return (mem_block_t *)(void *)((uint8_t *)ptr - mem_block_header_size());
}

/**
 * @brief 判断指针是否落在池载荷区域内。
 * @param[in] ptr 待检测的用户指针。
 * @return 在池内返回 1，否则返回 0。
 */
static int mem_ptr_in_pool(const void *ptr)
{
    const uint8_t *address = (const uint8_t *)ptr;

    if (address == 0) {
        return 0;
    }

    return (address >= (mem_pool_storage + mem_block_header_size())) &&
           (address < (mem_pool_storage + MEM_POOL_SIZE));
}

/**
 * @brief 返回含头部的块总字节跨度。
 * @param[in] block 待测量的块头部。
 * @return 含头部与载荷的总字节数。
 */
static size_t mem_block_span(const mem_block_t *block)
{
    return mem_block_header_size() + block->size;
}

/**
 * @brief 返回当前偏移处块之后下一块的字节偏移。
 * @param[in] offset 当前块偏移。
 * @return 池中下一块的字节偏移。
 */
static size_t mem_next_offset(size_t offset)
{
    const mem_block_t *block = mem_block_at(offset);

    return offset + mem_block_span(block);
}

/**
 * @brief 拆分空闲块，使前段具有所需载荷大小。
 * @param[in] offset 待拆分空闲块偏移。
 * @param[in] need 所需载荷字节数。
 */
static void mem_split_block(size_t offset, size_t need)
{
    mem_block_t *block = mem_block_at(offset);
    size_t remain = block->size - need;

    if (remain < (mem_block_header_size() + MEM_MIN_BLOCK)) {
        return;
    }

    block->size = need;

    offset += mem_block_span(block);
    block = mem_block_at(offset);
    block->size = remain - mem_block_header_size();
    block->used = 0U;
}

/**
 * @brief 从 offset 起合并连续空闲块。
 * @param[in] offset 合并链首块偏移。
 */
static void mem_coalesce_forward(size_t offset)
{
    size_t next;

    while ((next = mem_next_offset(offset)) < MEM_POOL_SIZE) {
        mem_block_t *block = mem_block_at(offset);
        mem_block_t *neighbor = mem_block_at(next);

        if ((block->used != 0U) || (neighbor->used != 0U)) {
            break;
        }

        block->size += mem_block_span(neighbor);
    }
}

/** @brief 合并池中所有相邻空闲块。 */
static void mem_coalesce_all(void)
{
    size_t offset = 0U;

    while (offset < MEM_POOL_SIZE) {
        mem_block_t *block = mem_block_at(offset);

        if (block->used == 0U) {
            mem_coalesce_forward(offset);
        }

        offset += mem_block_span(block);
    }
}

/** @brief 初始化内存池为单一空闲块。 */
void mem_pool_init(void)
{
    mem_block_t *block = mem_block_at(0U);

    block->size = MEM_POOL_SIZE - mem_block_header_size();
    block->used = 0U;
    mem_pool_ready = 1U;
}

/** @brief 首次分配时惰性初始化内存池。 */
static void mem_pool_init_once(void)
{
    if (mem_pool_ready == 0U) {
        mem_pool_init();
    }
}

/**
 * @brief 使用首次适配从池中分配块。
 * @param[in] size 请求的载荷字节数。
 * @return 载荷指针；无合适空闲块时返回 null。
 */
static void *mem_pool_malloc(size_t size)
{
    size_t need;
    size_t offset;

    mem_pool_init_once();

    if (size == 0U) {
        return 0;
    }

    need = mem_align_up(size);
    if (need < MEM_MIN_BLOCK) {
        need = MEM_MIN_BLOCK;
    }

    offset = 0U;
    while (offset < MEM_POOL_SIZE) {
        mem_block_t *block = mem_block_at(offset);

        if ((block->used == 0U) && (block->size >= need)) {
            mem_split_block(offset, need);
            block = mem_block_at(offset);
            block->used = 1U;
            return mem_block_payload(block);
        }

        offset += mem_block_span(block);
    }

    return 0;
}

/**
 * @brief 释放池分配并合并相邻空闲块。
 * @param[in] ptr mem_pool_malloc 返回的载荷指针；为 null 或不在池内时忽略。
 */
static void mem_pool_free(void *ptr)
{
    mem_block_t *block;

    mem_pool_init_once();

    if (ptr == 0) {
        return;
    }

    if (mem_ptr_in_pool(ptr) == 0) {
        return;
    }

    block = mem_block_from_payload(ptr);
    block->used = 0U;
    mem_coalesce_all();
}

/**
 * @brief 尽可能原地调整块大小，否则复制到新块。
 * @param[in] ptr 现有载荷；为 null 时等同分配。
 * @param[in] size 新载荷字节数。
 * @return 新或原载荷指针；失败时返回 null。
 */
static void *mem_pool_realloc(void *ptr, size_t size)
{
    mem_block_t *block;
    size_t need;
    size_t offset;
    size_t next;
    void *new_ptr;

    mem_pool_init_once();

    if (ptr == 0) {
        return mem_pool_malloc(size);
    }

    if (size == 0U) {
        mem_pool_free(ptr);
        return 0;
    }

    if (mem_ptr_in_pool(ptr) == 0) {
        return 0;
    }

    block = mem_block_from_payload(ptr);
    need = mem_align_up(size);
    if (need < MEM_MIN_BLOCK) {
        need = MEM_MIN_BLOCK;
    }

    if (block->size >= need) {
        offset = (size_t)((uint8_t *)(void *)block - mem_pool_storage);
        mem_split_block(offset, need);
        return ptr;
    }

    offset = (size_t)((uint8_t *)(void *)block - mem_pool_storage);
    next = mem_next_offset(offset);
    if (next < MEM_POOL_SIZE) {
        mem_block_t *neighbor = mem_block_at(next);
        size_t combined = block->size + mem_block_span(neighbor);

        if ((neighbor->used == 0U) && (combined >= need)) {
            block->size = combined;
            if (block->size > need) {
                mem_split_block(offset, need);
            }
            return ptr;
        }
    }

    new_ptr = mem_pool_malloc(size);
    if (new_ptr == 0) {
        return 0;
    }

    memcpy(new_ptr, ptr, (block->size < size) ? block->size : size);
    mem_pool_free(ptr);
    return new_ptr;
}

/** @brief 返回内存池总字节容量。
 * @return MEM_POOL_SIZE 的值。 */
size_t mem_pool_total(void)
{
    return MEM_POOL_SIZE;
}

/** @brief 返回已用块占用的字节数（含头部）。
 * @return 已用块跨度之和。 */
size_t mem_pool_used(void)
{
    size_t offset = 0U;
    size_t used = 0U;

    mem_pool_init_once();

    while (offset < MEM_POOL_SIZE) {
        const mem_block_t *block = mem_block_at(offset);

        if (block->used != 0U) {
            used += mem_block_header_size() + block->size;
        }

        offset += mem_block_span(block);
    }

    return used;
}

/** @brief 返回空闲块字节数（含头部）。
 * @return 空闲块跨度之和。 */
size_t mem_pool_free_bytes(void)
{
    size_t offset = 0U;
    size_t free_bytes = 0U;

    mem_pool_init_once();

    while (offset < MEM_POOL_SIZE) {
        const mem_block_t *block = mem_block_at(offset);

        if (block->used == 0U) {
            free_bytes += mem_block_header_size() + block->size;
        }

        offset += mem_block_span(block);
    }

    return free_bytes;
}

/** @brief 返回池中最大连续空闲载荷。
 * @return 最大空闲载荷字节数（不含块头）。 */
size_t mem_pool_largest_free(void)
{
    size_t offset = 0U;
    size_t largest = 0U;

    mem_pool_init_once();

    while (offset < MEM_POOL_SIZE) {
        const mem_block_t *block = mem_block_at(offset);

        if ((block->used == 0U) && (block->size > largest)) {
            largest = block->size;
        }

        offset += mem_block_span(block);
    }

    return largest;
}

/**
 * @brief newlib malloc 重载，委托 mem_pool_malloc。
 * @param[in] size 请求的载荷字节数。
 * @return 分配得到的指针；失败时返回 null。
 */
void *malloc(size_t size) __attribute__((used));
/**
 * @brief newlib free 重载，委托 mem_pool_free。
 * @param[in] ptr 待释放指针；为 null 时忽略。
 */
void free(void *ptr) __attribute__((used));
/**
 * @brief newlib realloc 重载，委托 mem_pool_realloc。
 * @param[in] ptr 现有指针；为 null 时等同 malloc。
 * @param[in] size 新载荷字节数。
 * @return 新或原指针；失败时返回 null。
 */
void *realloc(void *ptr, size_t size) __attribute__((used));
/**
 * @brief newlib calloc 重载，使用 mem_pool_malloc 并清零。
 * @param[in] count 元素个数。
 * @param[in] size 每个元素字节数。
 * @return 清零后的指针；失败时返回 null。
 */
void *calloc(size_t count, size_t size) __attribute__((used));
/**
 * @brief newlib 可重入 malloc 重载。
 * @param[in] reent 可重入上下文（未使用）。
 * @param[in] size 请求的载荷字节数。
 * @return 分配得到的指针；失败时返回 null。
 */
void *_malloc_r(struct _reent *reent, size_t size) __attribute__((used));
/**
 * @brief newlib 可重入 free 重载。
 * @param[in] reent 可重入上下文（未使用）。
 * @param[in] ptr 待释放指针。
 */
void _free_r(struct _reent *reent, void *ptr) __attribute__((used));
/**
 * @brief newlib 可重入 realloc 重载。
 * @param[in] reent 可重入上下文（未使用）。
 * @param[in] ptr 现有指针。
 * @param[in] size 新载荷字节数。
 * @return 新或原指针；失败时返回 null。
 */
void *_realloc_r(struct _reent *reent, void *ptr, size_t size) __attribute__((used));
/**
 * @brief newlib 可重入 calloc 重载。
 * @param[in] reent 可重入上下文（未使用）。
 * @param[in] count 元素个数。
 * @param[in] size 每个元素字节数。
 * @return 清零后的指针；失败时返回 null。
 */
void *_calloc_r(struct _reent *reent, size_t count, size_t size) __attribute__((used));
/**
 * @brief 通过 sbrk 禁止堆增长（仅固定池）。
 * @param[in] reent 可重入上下文（未使用）。
 * @param[in] incr 请求增量（未使用）。
 * @return 始终返回 (void *)-1 表示失败。
 */
void *_sbrk_r(struct _reent *reent, ptrdiff_t incr) __attribute__((used));

void *malloc(size_t size)
{
    return mem_pool_malloc(size);
}

void free(void *ptr)
{
    mem_pool_free(ptr);
}

void *realloc(void *ptr, size_t size)
{
    return mem_pool_realloc(ptr, size);
}

void *calloc(size_t count, size_t size)
{
    size_t total;
    void *ptr;

    if ((count != 0U) && (size > (SIZE_MAX / count))) {
        return 0;
    }

    total = count * size;
    ptr = mem_pool_malloc(total);
    if (ptr != 0) {
        memset(ptr, 0, total);
    }

    return ptr;
}

void *_malloc_r(struct _reent *reent, size_t size)
{
    (void)reent;
    return mem_pool_malloc(size);
}

void _free_r(struct _reent *reent, void *ptr)
{
    (void)reent;
    mem_pool_free(ptr);
}

void *_realloc_r(struct _reent *reent, void *ptr, size_t size)
{
    (void)reent;
    return mem_pool_realloc(ptr, size);
}

void *_calloc_r(struct _reent *reent, size_t count, size_t size)
{
    (void)reent;
    return calloc(count, size);
}

void *_sbrk_r(struct _reent *reent, ptrdiff_t incr)
{
    (void)reent;
    (void)incr;
    return (void *)-1;
}
