#include "allo.h"

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "stats.h"

/* // Arenas are allocated for all sizes <= 256 bytes. */
/* #define MAX_ARENA_SIZE (1 << 8) */
/* // How much arenas grow by */
/* #define ARENA_SIZE 4096 */

// How much sbrk grows by
#define SBRK_BY 4096
// directly mmap chunks of size > 2^19
#define NUM_SIZE_BUCKETS 19
#define MAX_NON_MMAP (1 << NUM_SIZE_BUCKETS)

#define MIN_ALLOC_SIZE (sizeof (struct free_chunk))

// sorted in an avl tree
typedef struct free_chunk {
    size_t size;
    uint64_t height;
    struct free_chunk *left;
    struct free_chunk *right;
} free_chunk;

typedef struct chunk {
    size_t size;
    char data[];
} chunk;

typedef struct mmap_node {
    struct mmap_node *prev;
    struct mmap_node *next;
    uint64_t free : 1;
    uint64_t count : 1;
    size_t size;
    char data[];
} mmap_node;

/* typedef struct arena_block { */
/*     struct arena_block *prev; */
/*     struct arena_block *next; */
/*     uint64_t status; */
/*     size_t size; */
/*     char data[ARENA_SIZE]; */
/* } arena_block; */

typedef struct allocator {
    free_chunk *size_buckets[NUM_SIZE_BUCKETS];
    stats stats;
    uint64_t highest_allocated;
    uint64_t heap_top;
    mmap_node *mmap_chunk_head;
    mmap_node *mmap_chunk_tail;
} allocator;

uint64_t round_to_alloc_size(size_t n) {
    if (n > MAX_NON_MMAP)
      return n + sizeof(mmap_node);

    size_t p = MIN_ALLOC_SIZE;
    while (p < n + sizeof(chunk))
        p <<= 1;
    return p;
}

void *allo_cate_mmaped(size_to to_alloc) {
    return NULL;
}

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
