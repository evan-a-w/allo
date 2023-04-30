#include "allo.h"

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>

#include "stats.h"
#include "rb_tree.h"

free_chunk *add_heap(allocator *a) {
    heap *h = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    h->prev = a->heap_tail;
    h->next = NULL;
    if (a->heap_tail != NULL) {
        a->heap_tail->next = h;
    }
    a->heap_tail = h;
    if (a->heap_head == NULL) {
        a->heap_head = h;
    }
    a->stats.total_heap_size += HEAP_SIZE;

    free_chunk *res = h->free_chunks;
    *res = (free_chunk){
        .status = HEAP_SIZE - sizeof(heap) - sizeof(chunk),
        .prev_absolute = NULL,
        .next_absolute = NULL,
    };
    return res;
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
    if (p < (1 << 7)) {
        return p;
    } else if (p <= MAX_ARENA_SIZE) {
        uint64_t pow_two = 1 << 7;
        while (pow_two < p) {
            pow_two <<= 1;
        }
        return pow_two;
    }

    return ROUND_SIZE_TO_ALIGN(n);
}

uint64_t get_arena_bucket(uint64_t status) {
    uint64_t size = ARENA_CHUNK_SIZE(status) - sizeof(chunk);
    if (size <= 128) {
        return (size - 16) / 8;
    }
    return 15 + 64 - __builtin_clzll(size - 1) - 7;
}

void *allo_cate_mmaped(allocator *a, size_t size) {
    size_t to_alloc = size + sizeof(struct mmapped_chunk);
    mmapped_chunk *c = mmap(NULL, to_alloc, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    c->status = size | MMAPPED;
    c->prev = NULL;
    c->next = NULL;
    a->stats.num_bytes_allocated += to_alloc;
    return c->data;
}

void *allo_cate_arena(allocator *a, size_t to_alloc) {
    size_t status = to_alloc | ARENA;
    return NULL;
}

heap *round_to_heap(void *p) {
    return (heap *)((uint64_t)p & ~(HEAP_SIZE - 1));
}

void *allo_cate(allocator *a, size_t size) {
    size_t to_alloc = round_to_alloc_size_without_metadata(size);
    if (to_alloc <= MAX_ARENA_SIZE) {
        return allo_cate_arena(a, to_alloc);
    } else if (to_alloc >= MIN_MMAP) {
        return allo_cate_mmaped(a, to_alloc);
    }

    // standard allocation
    free_chunk_tree *best_fit_node = rb_tree_search(a->free_chunk_tree, size);

    free_chunk *best_fit;
    if (best_fit_node == NULL) {
        best_fit = add_heap(a);
    } else if (best_fit_node->next_of_size == NULL) {
        best_fit = (free_chunk *)best_fit_node;
        a->free_chunk_tree = rb_tree_remove(a->free_chunk_tree,
                                            CHUNK_SIZE(best_fit_node->status));
    } else {
        best_fit = (free_chunk *)best_fit_node->next_of_size;
        best_fit_node->next_of_size = best_fit_node->next_of_size->next_of_size;
        if (best_fit_node->next_of_size != NULL) {
            best_fit_node->next_of_size->prev_of_size =
                (free_chunk *)best_fit_node;
        }
    }

    // try split node
    size_t leftover = CHUNK_SIZE(best_fit->status) - to_alloc - sizeof(chunk);
    size_t rounded_down = CHUNK_SIZE_ALIGN * (leftover / CHUNK_SIZE_ALIGN);
    if (rounded_down > MAX_ARENA_SIZE) {
        best_fit->status &= ~(CHUNK_SIZE_ALIGN - 1);
        size_t new_size = to_alloc + (leftover - rounded_down);
        best_fit->status |= new_size;

        free_chunk *split_chunk =
            (free_chunk *)((uint64_t)((chunk *)best_fit)->data + new_size);
        split_chunk->status = rounded_down | FREE;
        split_chunk->prev_absolute = (chunk *)best_fit;
        split_chunk->next_absolute = best_fit->next_absolute;
        if (best_fit->next_absolute != NULL
            && best_fit->next_absolute->status & FREE) {
            free_chunk *next_absolute = (free_chunk *)best_fit->next_absolute;
            next_absolute->prev_absolute = (chunk *)split_chunk;
        }
        a->free_chunk_tree =
            rb_tree_insert(a->free_chunk_tree, (free_chunk_tree *)split_chunk);
    }

    best_fit->status = to_alloc & ~FREE;
    a->stats.num_bytes_allocated += to_alloc;

    return ((chunk *)best_fit)->data;
}

int main(void) {
    printf("Hello, World!\n");
    return 0;
}
