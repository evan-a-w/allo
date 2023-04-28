#include "allo.h"

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>

#include "stats.h"

// Arenas are allocated for all sizes <= 1024 bytes.
#define MAX_ARENA_SIZE (1 << 10)

#define PAGE_SIZE 4096

// How much arenas grow by
#define ARENA_SIZE 4096

// each heap
#define HEAP_SIZE (PAGE_SIZE * 32)

// 16 bytes
#define MIN_ALLOC_SIZE (sizeof(struct arena_free_chunk))

// directly mmap requests >= this size
#define MIN_MMAP (HEAP_SIZE - sizeof(struct heap) - PAGE_SIZE)

// 5 bits of flags
#define CHUNK_ALIGN (1 << 5)
// 4 bits of flags
#define ARENA_ALIGN (1 << 4)

#define ROUND_TO_ALIGN(x) (((x) + CHUNK_ALIGN - 1) & ~(CHUNK_ALIGN - 1))

#define CHUNK_SIZE(status) ((status) & ~(CHUNK_ALIGN - 1))

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

enum chunk_status {
    ARENA = 1,
    RED = 2,
    TREE = 4,
    MMAPPED = 8,
};

#define IS_ARENA(status) ((status)&ARENA)
#define IS_RED(status) ((status)&RED)
#define IS_TREE(status) ((status)&TREE)
#define IS_MMAPPED(status) ((status)&MMAPPED)

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
    struct arena_free_chunk *next;
} arena_free_chunk;

// malloc larger blocks to subdivide for the arenas
typedef struct arena_block {
    uint64_t status;
    struct arena_block *prev;
    struct arena_block *next;
    struct arena_free_chunk *free_list;
} arena_block;

typedef struct heap {
    struct heap *prev;
    struct heap *next;
    uint64_t allocated_bytes;
} heap;

heap *round_to_heap(void *p) {
    return (heap *) ((uint64_t) p & ~(HEAP_SIZE - 1));
}

typedef struct allocator {
    stats stats;
    heap *heap_list;
} allocator;

void add_heap(allocator *a) {
    heap *h = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    h->next = a->heap_list;
    h->prev = NULL;
    if (a->heap_list != NULL) {
        a->heap_list->prev = h;
    }
    a->heap_list = h;
    a->stats.total_heap_size += HEAP_SIZE;
}

uint64_t round_to_alloc_size_without_metadata(size_t n) {
    if (n >= MIN_MMAP)
        return n;
    if (n < MIN_ALLOC_SIZE)
        return MIN_ALLOC_SIZE;

    // smallest x such that 16 + 8x >= n
    // ceil((n - 16) / 8)
    size_t x = (n - MIN_ALLOC_SIZE + 7) / 8;
    size_t p = MIN_ALLOC_SIZE + 8 * x;
    if (p <= MAX_ARENA_SIZE) {
        return p;
    }

    return ROUND_TO_ALIGN(n);
}

void *allo_cate_mmaped(allocator *a, size_t to_alloc) {
    to_alloc = ROUND_TO_ALIGN(to_alloc + sizeof(struct mmapped_chunk));
    mmapped_chunk *c = mmap(NULL, to_alloc, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    c->status = to_alloc | MMAPPED;
    c->prev = NULL;
    c->next = NULL;
    a->stats.num_bytes_allocated += to_alloc;
    return c->data;
}

void *allo_cate(allocator *a, size_t size) {
    size_t to_alloc = round_to_alloc_size_without_metadata(size);
    if (to_alloc >= MIN_MMAP) {
        return allo_cate_mmaped(a, to_alloc);
    }
    return NULL;
}

int main(void) {
    printf("Hello, World!\n");
    return 0;
}
