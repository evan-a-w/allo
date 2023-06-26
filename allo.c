#include "allo.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <assert.h>

#include "avl_tree/avl_tree.h"
#include "stats.h"

#ifdef __ALLO_DEBUG_PRINT
#include <stdarg.h>
#endif

void debug_printf(const char *fmt, ...) {
#ifdef __ALLO_DEBUG_PRINT
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#else
    (void)fmt;
#endif
}

chunk *to_chunk(void *p) { return (chunk *)((char *)p - sizeof(chunk)); }

heap_chunk *to_heap_chunk(void *p) {
    return (heap_chunk *)((char *)p - sizeof(heap_chunk));
}

void free_chunk_init(free_chunk *res, size_t size, heap_chunk *prev,
                     size_t status_bits) {
    *(free_chunk_tree *)res = (free_chunk_tree){
        .prev = prev,
        .status = size | status_bits,
        .height = 0,
        .next_of_size = NULL,
        .child = {0},
    };
}

free_chunk *add_heap(allocator *a) {
    heap *h = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (h == MAP_FAILED)
        return NULL;
    h->next = a->heaps;
    h->prev = NULL;
    if (a->heaps != NULL)
        a->heaps->prev = h;
    a->heaps = h;
    a->stats.total_heap_size += HEAP_SIZE;

    h->end_of_heap = (uint64_t)h + HEAP_SIZE;

    free_chunk *res = (free_chunk *)h->free_chunks;

    size_t chunk_size = HEAP_SIZE - sizeof(heap) - sizeof(heap_chunk);
    assert(chunk_size == CHUNK_SIZE(chunk_size));

    free_chunk_init(res, chunk_size, 0, 0);

    debug_printf("add_heap: %p\n", h);

    return res;
}

heap_chunk *next_chunk(allocator *a, heap_chunk *c);

void print_allocator_state(allocator *a) {
    printf("allocator state:\n");
    printf("  heaps:\n");
    for (heap *h = a->heaps; h != NULL; h = h->next) {
        printf("    %p\n", (void *)h);
    }
    printf("  mmapped chunks:\n");
    for (mmapped_chunk *c = a->mmapped_chunk_head; c != NULL; c = c->next) {
        printf("    %p\n", (void *)c);
    }
    printf("  heap chunks:\n");
    for (heap *h = a->heaps; h != NULL; h = h->next) {
        for (free_chunk *c = (free_chunk *)h->free_chunks; c != NULL;
             c = next_chunk(a, c))
            debug_printf("      %zu %p (%s)\n", SIZE(c), (void *)c,
                         (c->status & FREE) ? "free" : "used");
    }
}

uint64_t round_to_alloc_size_without_metadata(size_t n) {
    if (n >= MIN_MMAP)
        return n;
    if (n < MIN_ALLOC_SIZE)
        return MIN_ALLOC_SIZE;

    // smallest x such that 16 + 8x >= n
    // ceil((n - 16) / 8)
    size_t x = (n - MIN_ALLOC_SIZE + (ARENA_SIZE_ALIGN - 1)) / ARENA_SIZE_ALIGN;
    size_t p = MIN_ALLOC_SIZE + ARENA_SIZE_ALIGN * x;
    if (p <= ARENA_DOUBLING_SIZE) {
        return p;
    } else if (p <= MAX_ARENA_SIZE) {
        uint64_t pow_two = ARENA_DOUBLING_SIZE;
        while (pow_two < p) {
            pow_two <<= 1;
        }
        return pow_two;
    }

    return ROUND_SIZE_TO_ALIGN(n);
}

void *allo_cate_mmaped(allocator *a, size_t size) {
    debug_printf("allo_cate_mmaped: %lu\n", size);
    size_t to_alloc = size + sizeof(struct mmapped_chunk);
    mmapped_chunk *c = mmap(NULL, to_alloc, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    // need accurate allocation size because munmap requires size
    c->status = to_alloc | MMAPPED;
    c->prev = NULL;
    c->next = a->mmapped_chunk_head;
    if (a->mmapped_chunk_head != NULL) {
        a->mmapped_chunk_head->prev = c;
    }
    a->stats.num_bytes_allocated += to_alloc;
    return c->data;
}

uint64_t get_arena_bucket(uint64_t status) {
    uint64_t size = ARENA_CHUNK_SIZE(status);
    if (size <= ARENA_DOUBLING_SIZE)
        return (size - MIN_ALLOC_SIZE) / ARENA_SIZE_ALIGN;

    int64_t pow_two = sizeof(uint64_t) * 8 - __builtin_clzll(size - 1);
    return pow_two - ARENA_DOUBLING_POWER
           + (ARENA_DOUBLING_SIZE - MIN_ALLOC_SIZE) / ARENA_SIZE_ALIGN;
}

size_t arena_block_size(size_t size) {
    size_t arena_size = size * ARENA_GROWTH_FACTOR + sizeof(arena_block);
    if (arena_size <= MAX_ARENA_SIZE) {
        arena_size = ROUND_SIZE_TO_ALIGN(MAX_ARENA_SIZE + sizeof(arena_block));
    }
    return arena_size;
}

void *allo_cate_arena(allocator *a, size_t to_alloc) {
    void *res;

    // changes other thing in tree between
    uint64_t bucket = get_arena_bucket(to_alloc);
    arena *arena = &a->arenas[bucket];
    debug_printf("allo_cate_arena: %lu %lu\n", to_alloc, bucket);

try_take_from_free_list:
    if (arena->free_list) {
        arena_free_chunk *c = arena->free_list;
        arena->free_list = c->next;
        chunk *ch = (chunk *)c;
        ch->status = to_alloc;
        res = ch->data;
        goto end;
    }

    size_t arena_size = arena_block_size(to_alloc);
    arena_block *block = allo_cate(a, arena_size);
    block->prev = NULL;
    block->next = arena->arena_block_head;
    if (arena->arena_block_head != NULL)
        arena->arena_block_head->prev = block;
    arena->arena_block_head = block;
    if (block == NULL) {
        res = NULL;
        goto end;
    }

    uint64_t end_of_block = (uint64_t)block + arena_size;
    arena_free_chunk *c = (arena_free_chunk *)block->data;
    while ((uint64_t)c + sizeof(chunk) + to_alloc < end_of_block) {
        c->status = to_alloc | FREE;
        c->next = arena->free_list;
        arena->free_list = c;

        c = (arena_free_chunk *)(((chunk *)c)->data + to_alloc);
    }

    goto try_take_from_free_list;
end:
    avl_tree_debug_print(a->free_chunk_tree);
    debug_printf("END allo_cate_arena: %lu %lu\n", to_alloc,
                 get_arena_bucket(to_alloc));
    return res;
}

heap *get_heap(allocator *a, void *p) {
    for (heap *h = a->heaps; h != NULL; h = h->next) {
        if ((uint64_t)h <= (uint64_t)p && (uint64_t)p < h->end_of_heap)
            return h;
    }
    return NULL;
}

bool same_heap(allocator *a, void *p1, void *p2) {
    return get_heap(a, p1) == get_heap(a, p2);
}

heap_chunk *next_chunk(allocator *a, heap_chunk *c) {
    heap_chunk *next = (heap_chunk *)(c->data + CHUNK_SIZE(c->status));
    next = same_heap(a, c, next) ? next : NULL;
    debug_printf("next of %p is %p\n", c, next);
    return next;
}

heap_chunk *prev_chunk(heap_chunk *c) {
    debug_printf("prev of %p is %p\n", c, c->prev);
    return c->prev;
}

// chunk is not yet in the tree
void coalesce(allocator *a, heap_chunk *chunk) {
#ifdef ALLO_AVL_DEBUG
    assert(!avl_tree_contains(a->free_chunk_tree, chunk));
#endif

    uint64_t size = CHUNK_SIZE(chunk->status);
    debug_printf("coalesce: %p (size %lu)\n", chunk, size);

    heap_chunk *prev = prev_chunk(chunk);
    if (prev && IS_FREE(prev->status)) {
        a->free_chunk_tree = avl_tree_remove_node(a->free_chunk_tree, prev);
        size += CHUNK_SIZE(prev->status) + sizeof(heap_chunk);
        chunk = prev;
        chunk->status = size;
    }

    assert(size == CHUNK_SIZE(size));

    heap_chunk *next_absolute = next_chunk(a, chunk);
    if (next_absolute && IS_FREE(next_absolute->status)) {
        heap_chunk *next_again = next_chunk(a, next_absolute);
        if (next_again)
            next_again->prev = chunk;
        a->free_chunk_tree =
            avl_tree_remove_node(a->free_chunk_tree, next_absolute);
        size += CHUNK_SIZE(next_absolute->status) + sizeof(heap_chunk);
    } else if (next_absolute) {
        next_absolute->prev = chunk;
    }

    assert(size == CHUNK_SIZE(size));

    free_chunk_init(chunk, size, chunk->prev, FREE | TREE);

    a->free_chunk_tree =
        avl_tree_insert(a->free_chunk_tree, (free_chunk_tree *)chunk);
    avl_tree_debug_print(a->free_chunk_tree);

    debug_printf("END coalesce: %p (size %lu)\n", chunk,
                 CHUNK_SIZE(chunk->status));
}

void *allo_cate_standard(allocator *a, size_t to_alloc) {
    debug_printf("allo_cate: standard %lu\n", to_alloc);
    free_chunk *best_fit = avl_tree_search(a->free_chunk_tree, to_alloc);

    if (best_fit == NULL) {
        best_fit = add_heap(a);
        if (best_fit == NULL)
            return NULL;
    } else {
        a->free_chunk_tree = avl_tree_remove_node(a->free_chunk_tree, best_fit);
    }

    best_fit->status &= ~(FREE | TREE);

    // try split node
    size_t leftover = CHUNK_SIZE(best_fit->status) - to_alloc;
    if (leftover > sizeof(heap_chunk) + MAX_ARENA_SIZE) {
        leftover -= sizeof(heap_chunk);
        assert(leftover == CHUNK_SIZE(leftover));

        size_t new_size = CHUNK_SIZE(best_fit->status) - leftover;
        assert(new_size == CHUNK_SIZE(new_size));

        debug_printf("Split node of size %lu into %lu and %lu\n",
                     CHUNK_SIZE(best_fit->status), new_size, leftover);

        free_chunk *split_chunk = (free_chunk *)(best_fit->data + new_size);
        free_chunk_init(split_chunk, leftover, best_fit, FREE | TREE);

        coalesce(a, split_chunk);

        best_fit->status = new_size;
    }

    a->stats.num_bytes_allocated += CHUNK_SIZE(best_fit->status);

    return best_fit->data;
}

void *allo_cate(allocator *a, size_t size) {
    debug_printf("allo_cate: %lu\n", size);
    avl_tree_debug_print(a->free_chunk_tree);

    void *res = NULL;

    size_t to_alloc = round_to_alloc_size_without_metadata(size);
    if (to_alloc <= MAX_ARENA_SIZE) {
        res = allo_cate_arena(a, to_alloc);
    } else if (to_alloc >= MIN_MMAP) {
        res = allo_cate_mmaped(a, to_alloc);
    } else {
        res = allo_cate_standard(a, to_alloc);
    }

    debug_printf("allo_cate result: %p\n", res);
    avl_tree_debug_print(a->free_chunk_tree);
    debug_printf("END allo_cate: %lu\n", size);

    return res;
}

void allo_free_arena(allocator *a, chunk *ch) {
    if (ch == NULL)
        return;
    debug_printf("allo_free_arena: %lu\n", ARENA_CHUNK_SIZE(ch->status));
    arena *arena = &a->arenas[get_arena_bucket(ARENA_CHUNK_SIZE(ch->status))];
    ch->status |= FREE;
    arena_free_chunk *c = (arena_free_chunk *)ch;
    c->next = arena->free_list;
    arena->free_list = c;
}

void allo_free_mmaped(allocator *a, void *p) {
    mmapped_chunk *c = (mmapped_chunk *)((char *)p - sizeof(mmapped_chunk));
    debug_printf("allo_free_mmaped: %lu\n", CHUNK_SIZE(c->status));

    if (c->prev) {
        c->prev->next = c->next;
    } else {
        a->mmapped_chunk_head = c->next;
    }

    if (c->next)
        c->next->prev = c->prev;

    size_t size = CHUNK_SIZE(c->status);
    a->stats.num_bytes_allocated -= size;
    munmap(c, size);
}

void allo_free_standard(allocator *a, void *p) {
    heap_chunk *ch = to_heap_chunk(p);
    debug_printf("allo_free_standard: %lu\n", CHUNK_SIZE(ch->status));
    ch->status |= FREE;
    coalesce(a, ch);
}

void allo_free(allocator *a, void *p) {
    if (p == NULL)
        return;
    chunk *c = to_chunk(p);

    if (ARENA_CHUNK_SIZE(c->status) <= MAX_ARENA_SIZE) {
        allo_free_arena(a, c);
    } else if (c->status & MMAPPED) {
        allo_free_mmaped(a, p);
    } else {
        allo_free_standard(a, p);
    }
}

void initialize_allocator(allocator *a) {
    a->heaps = NULL;
    a->free_chunk_tree = NULL;
    a->mmapped_chunk_head = NULL;
    initialize_stats(&a->stats);
    for (size_t i = 0; i < NUM_ARENA_BUCKETS; i++) {
        a->arenas[i].arena_block_head = NULL;
        a->arenas[i].free_list = NULL;
    }
}

void free_allocator(allocator *a) {
    for (size_t i = 0; i < NUM_ARENA_BUCKETS; i++) {
        arena *arena = &a->arenas[i];
        arena_block *block_next = NULL;
        for (arena_block *b = arena->arena_block_head; b != NULL;
             b = block_next) {
            block_next = b->next;
            allo_free(a, b);
        }
        arena->arena_block_head = NULL;
        arena->free_list = NULL;
    }

    heap *heap_next;
    for (heap *h = a->heaps; h != NULL; h = heap_next) {
        heap_next = h->next;
        munmap(h, HEAP_SIZE);
    }
    mmapped_chunk *chunk_next;
    for (mmapped_chunk *c = a->mmapped_chunk_head; c != NULL; c = chunk_next) {
        chunk_next = c->next;
        munmap(c, CHUNK_SIZE(c->status));
    }
    a->free_chunk_tree = NULL;
    a->mmapped_chunk_head = NULL;
    a->heaps = NULL;
    initialize_stats(&a->stats);

    avl_tree_debug_print(a->free_chunk_tree);
}

size_t introspect_size(void *p) {
    return CHUNK_SIZE(((chunk *)((char *)p - sizeof(chunk)))->status);
}

// malloc etc.
allocator global_allocator = {0};

void *_allo_malloc(size_t size) { return allo_cate(&global_allocator, size); }

void _allo_free(void *p) { allo_free(&global_allocator, p); }

void *_allo_realloc(void *p, size_t size) {
    if (p == NULL)
        return _allo_malloc(size);
    if (introspect_size(p) >= size)
        return p;
    void *new_p = _allo_malloc(size);
    memcpy(new_p, p, introspect_size(p));
    _allo_free(p);
    return new_p;
}

void *_allo_calloc(size_t nmemb, size_t size) {
    printf("CALLOC\n");
    void *p = _allo_malloc(nmemb * size);
    memset(p, 0, nmemb * size);
    return p;
}
