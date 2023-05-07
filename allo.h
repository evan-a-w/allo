#ifndef ALLO_H
#define ALLO_H

#include <stddef.h>
#include <stdint.h>

#include "stats.h"

#define __ALLO_DEBUG_PRINT
#define ALLO_OVERRIDE_MALLOC

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

// 5 bits of flags, 4th bit unused
#define CHUNK_SIZE_ALIGN (1 << 5)

// 3 bits of flags, 1st and 2nd unused
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

// should be size 32 so alignment isn't aids
// I guess size_t is bad
typedef struct heap_chunk {
    uint64_t padding[2];
    struct heap_chunk *prev;
    size_t status;
    char data[];
} heap_chunk;

typedef heap_chunk free_chunk;

enum chunk_status {
    FREE = 1,
    RED = 2,
    TREE = 4,
    MMAPPED = 8,
};

#define IS_ARENA(status) ((status)&ARENA)
#define IS_RED(status) ((status)&RED)
#define IS_TREE(status) ((status)&TREE)
#define IS_FREE(status) ((status)&FREE)
#define IS_MMAPPED(status) ((status)&MMAPPED)

// sorted in an rb tree
typedef struct free_chunk_tree {
    uint64_t padding[2];
    heap_chunk *prev;
    size_t status;
    struct free_chunk_list *next_of_size;
    struct free_chunk_tree *left;
    struct free_chunk_tree *right;
} free_chunk_tree;

typedef struct free_chunk_list {
    uint64_t padding[2];
    heap_chunk *prev;
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
    uint64_t end_of_heap;
    char free_chunks[];
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
size_t introspect_size(void *p);

void debug_printf(const char *fmt, ...);

// malloc etc.
extern allocator global_allocator;

void *_allo_malloc(size_t size);
void _allo_free(void *p);
void *_allo_calloc(size_t nmemb, size_t size);
void *_allo_realloc(void *ptr, size_t size);

#ifdef ALLO_OVERRIDE_MALLOC
#define malloc(x) _allo_malloc(x)
#define free(x) _allo_free(x)
#define calloc(x, y) _allo_calloc(x, y)
#define realloc(x, y) _allo_realloc(x, y)
#endif

#endif
