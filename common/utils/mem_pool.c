#include "mem_pool.h"
#include <reent.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MEM_ALIGN 4U
#define MEM_MIN_BLOCK 8U

typedef struct mem_block {
    size_t size;
    uint8_t used;
} mem_block_t;

static uint8_t mem_pool_storage[MEM_POOL_SIZE] __attribute__((aligned(MEM_ALIGN), used));
static uint8_t mem_pool_ready __attribute__((used));

static size_t mem_block_header_size(void)
{
    return (sizeof(mem_block_t) + (MEM_ALIGN - 1U)) & ~(MEM_ALIGN - 1U);
}

static size_t mem_align_up(size_t value)
{
    return (value + (MEM_ALIGN - 1U)) & ~(MEM_ALIGN - 1U);
}

static mem_block_t *mem_block_at(size_t offset)
{
    return (mem_block_t *)(void *)(mem_pool_storage + offset);
}

static void *mem_block_payload(mem_block_t *block)
{
    return (uint8_t *)(void *)block + mem_block_header_size();
}

static mem_block_t *mem_block_from_payload(void *ptr)
{
    return (mem_block_t *)(void *)((uint8_t *)ptr - mem_block_header_size());
}

static int mem_ptr_in_pool(const void *ptr)
{
    const uint8_t *address = (const uint8_t *)ptr;

    if (address == 0) {
        return 0;
    }

    return (address >= (mem_pool_storage + mem_block_header_size())) &&
           (address < (mem_pool_storage + MEM_POOL_SIZE));
}

static size_t mem_block_span(const mem_block_t *block)
{
    return mem_block_header_size() + block->size;
}

static size_t mem_next_offset(size_t offset)
{
    const mem_block_t *block = mem_block_at(offset);

    return offset + mem_block_span(block);
}

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

void mem_pool_init(void)
{
    mem_block_t *block = mem_block_at(0U);

    block->size = MEM_POOL_SIZE - mem_block_header_size();
    block->used = 0U;
    mem_pool_ready = 1U;
}

static void mem_pool_init_once(void)
{
    if (mem_pool_ready == 0U) {
        mem_pool_init();
    }
}

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

size_t mem_pool_total(void)
{
    return MEM_POOL_SIZE;
}

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

void *malloc(size_t size) __attribute__((used));
void free(void *ptr) __attribute__((used));
void *realloc(void *ptr, size_t size) __attribute__((used));
void *calloc(size_t count, size_t size) __attribute__((used));
void *_malloc_r(struct _reent *reent, size_t size) __attribute__((used));
void _free_r(struct _reent *reent, void *ptr) __attribute__((used));
void *_realloc_r(struct _reent *reent, void *ptr, size_t size) __attribute__((used));
void *_calloc_r(struct _reent *reent, size_t count, size_t size) __attribute__((used));
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
