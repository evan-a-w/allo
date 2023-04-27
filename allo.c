#include "allo.h"

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "stats.h"

// Arenas are allocated for all sizes <= 1024 bytes.
#define MAX_ARENA_SIZE (1 << 10)
// How much arenas grow by
#define ARENA_SIZE 4096

// How much sbrk grows by
#define SBRK_BY 4096
// directly mmap chunks of size > 2^19
#define NUM_SIZE_BUCKETS 19
#define MAX_NON_MMAP (1 << NUM_SIZE_BUCKETS)

// 16 bytes
#define MIN_ALLOC_SIZE (sizeof(struct arena_free_chunk))

// 5 bits of flags
#define CHUNK_ALIGN (1 << 5)
// 4 bits of flags
#define ARENA_ALIGN (1 << 4)

#define ROUND_TO_ALIGN(x) (((x) + CHUNK_ALIGN - 1) & ~(CHUNK_ALIGN - 1))

#define SIZE(status) ((status) & ~(CHUNK_ALIGN - 1))
#define RB_COLOUR(status) ((status)&2)

typedef struct chunk {
    size_t status;
    char data[];
} chunk;

enum chunk_status {
    USED = 0,
    FREE = 1,
    RED = 2,
    BLACK = 0,
    TREE = 4,
    LIST = 0,
    SEEN_AS_FREE = 8,
    NOT_YET_SEEN_AS_FREE = 0,
};

#define IS_TREE(status) ((status)&TREE == TREE)
#define IS_RED(status) ((status)&RED == RED)
#define IS_FREE(status) ((status)&FREE == FREE)

typedef struct free_chunk {
    size_t status;
    struct chunk *prev_absolute;
} free_chunk;

// sorted in an rb tree
typedef struct free_chunk_tree {
    size_t status;
    struct chunk *prev_absolute;
    struct free_chunk_list *next_of_size;
    struct free_chunk_tree *left;
    struct free_chunk_tree *right;
} free_chunk_tree;

typedef struct free_chunk_list {
    size_t status;
    struct chunk *prev_absolute;
    struct free_chunk_list *next_of_size;
    struct free_chunk *prev_of_size;
} free_chunk_list;

typedef struct arena_free_chunk {
    size_t size;
    struct arena_chunk *next;
} arena_chunk;

typedef struct arena_block {
    struct arena_block *prev;
    struct arena_block *next;
    uint64_t status;
    size_t size;
    char data[ARENA_SIZE];
} arena_block;

typedef struct allocator {
    stats stats;
    uint64_t highest_allocated;
    uint64_t heap_top;
} allocator;

uint64_t round_to_alloc_size(size_t n) {
    n += sizeof(chunk);
    if (n > MAX_NON_MMAP)
        return n;

    size_t p = MIN_ALLOC_SIZE;
    while (p < n && p < MAX_ARENA_SIZE)
        p <<= 1;

    if (p <= MAX_ARENA_SIZE)
        return p;

    return ROUND_TO_ALIGN(n);
}

void *allo_cate_mmaped(size_t to_alloc) { return NULL; }

void *allo_cate(size_t size) {
    size_t to_alloc = round_to_alloc_size(size);
    if (to_alloc > MAX_NON_MMAP) {
        return allo_cate_mmaped(to_alloc);
    }
    return NULL;
}

int main(void) {
    printf("Hello, World!\n");
    return 0;
}
