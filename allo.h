#ifndef ALLO_H
#define ALLO_H

#include <stdint.h>
#include <stddef.h>

#include "stats.h"

#define __ALLO_DEBUG_PRINT

// Arenas are allocated for all sizes <= 1024 bytes.
// sizes are powers of two when >= 124 bytes
#define MAX_ARENA_POWER (10)
#define MAX_ARENA_SIZE (1 << MAX_ARENA_POWER)
#define ARENA_DOUBLING_POWER (7)
#define ARENA_DOUBLING_SIZE (1 << ARENA_DOUBLING_POWER)
#define NUM_ARENA_BUCKETS                                                      \
    ((ARENA_DOUBLING_SIZE / 8) - (MIN_ALLOC_SIZE / 8 - 1)                      \
     + (MAX_ARENA_POWER - ARENA_DOUBLING_POWER))

#define PAGE_SIZE 4096

#define ARENA_GROWTH_FACTOR 16

// each heap
#define HEAP_SIZE (PAGE_SIZE * 32)

// 16 bytes
#define MIN_ALLOC_SIZE (sizeof(struct arena_free_chunk))

// directly mmap requests >= this size
// size at which you can't store two of the same thing on a heap
// (x + sizeof(chunk)) * 2 > HEAP_SIZE
// x > HEAP_SIZE / 2 - sizeof(chunk)
#define MIN_MMAP ((HEAP_SIZE - sizeof(struct heap)) / 2 - sizeof(struct chunk))

// 5 bits of flags
#define CHUNK_SIZE_ALIGN (1 << 5)
// 3 bits of flags
#define ARENA_SIZE_ALIGN (1 << 3)

#define ROUND_SIZE_TO_ALIGN(x)                                                 \
    (((x) + CHUNK_SIZE_ALIGN - 1) & ~(CHUNK_SIZE_ALIGN - 1))

#define CHUNK_SIZE(status) ((status) & ~(CHUNK_SIZE_ALIGN - 1))
#define ARENA_CHUNK_SIZE(status) ((status) & ~(ARENA_SIZE_ALIGN - 1))

typedef struct chunk {
    size_t status;
    char data[];
} chunk;

typedef struct mmapped_chunk {
    struct mmapped_chunk *prev;
    struct mmapped_chunk *next;
    size_t status;
    char data[];
} mmapped_chunk;

typedef struct heap_chunk {
    size_t prev_size; // 0 if no prev
    size_t status;
    char data[];
} heap_chunk;

typedef heap_chunk free_chunk;

enum chunk_status {
    ARENA = 1,
    FREE = 2,
    RED = 4,
    TREE = 8,

    MMAPPED = 16,
};

#define IS_ARENA(status) ((status)&ARENA)
#define IS_RED(status) ((status)&RED)
#define IS_TREE(status) ((status)&TREE)
#define IS_FREE(status) ((status)&FREE)
#define IS_MMAPPED(status) ((status)&MMAPPED)

// sorted in an rb tree
typedef struct free_chunk_tree {
    size_t prev_size;
    size_t status;
    struct free_chunk_list *next_of_size;
    struct free_chunk_tree *left;
    struct free_chunk_tree *right;
} free_chunk_tree;

typedef struct free_chunk_list {
    size_t prev_size;
    size_t status;
    struct free_chunk_list *next_of_size;
    free_chunk *prev_of_size;
} free_chunk_list;

typedef struct arena_free_chunk {
    size_t status;
    struct arena_free_chunk *next;
} arena_free_chunk;

// malloc larger blocks to subdivide for the arenas
typedef struct arena_block {
    struct arena_block *prev;
    struct arena_block *next;
    char data[];
} arena_block;

typedef struct arena {
    struct arena_block *arena_block_head;
    struct arena_free_chunk *free_list;
} arena;

typedef struct heap {
    struct heap *prev;
    struct heap *next;
    uint64_t allocated_bytes;
    free_chunk free_chunks[];
} heap;

typedef struct allocator {
    stats stats;
    heap *heaps;
    mmapped_chunk *mmapped_chunk_head;
    free_chunk_tree *free_chunk_tree;
    arena arenas[NUM_ARENA_BUCKETS];
} allocator;

void initialize_allocator(allocator *a);
void free_allocator(allocator *a);

void *allo_cate(allocator *a, size_t size);
void allo_free(allocator *a, void *p);

void debug_printf(const char *fmt, ...);

#endif
