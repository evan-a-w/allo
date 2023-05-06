#include "allo.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "rb_tree.h"
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

void free_chunk_init(free_chunk *res, size_t size, size_t prev_size) {
    *(free_chunk_tree *)res = (free_chunk_tree){
        .prev_size = prev_size,
        .status = size | FREE,
        // set these to null even though its not a tree because we forget to do
        // this elsewhere <3
        .next_of_size = NULL,
        .left = NULL,
        .right = NULL,
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

    free_chunk *res = (free_chunk *)h->free_chunks;

    size_t chunk_size = HEAP_SIZE - sizeof(heap) - sizeof(heap_chunk);
    chunk_size &= ~(CHUNK_SIZE_ALIGN - 1);

    free_chunk_init(res, chunk_size, 0);

    debug_printf("add_heap: %p\n", res);

    return res;
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
    arena *arena = &a->arenas[get_arena_bucket(to_alloc)];
    debug_printf("allo_cate_arena: %lu %lu\n", to_alloc,
                 get_arena_bucket(to_alloc));

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
    if (block == NULL) {
        res = NULL;
        goto end;
    }

    uint64_t end_of_block = (uint64_t)block + arena_size;
    arena->free_list = NULL;
    for (arena_free_chunk *c = (arena_free_chunk *)block->data;
         (uint64_t)c < end_of_block;
         c = (arena_free_chunk *)(((chunk *)c)->data + to_alloc)) {
        c->status = to_alloc | FREE;
        c->next = arena->free_list;
        arena->free_list = c;
    }

    goto try_take_from_free_list;
end:
    rb_tree_debug_print(a->free_chunk_tree);
    debug_printf("END allo_cate_arena: %lu %lu\n", to_alloc,
                 get_arena_bucket(to_alloc));
    return res;
}

heap *round_to_heap(void *p) {
    return (heap *)((uint64_t)p & ~(HEAP_SIZE - 1));
}

bool same_heap(void *p1, void *p2) {
    return (uint64_t)round_to_heap(p1) == (uint64_t)round_to_heap(p2);
}

heap_chunk *next_chunk(heap_chunk *c) {
    heap_chunk *next = (heap_chunk *)(c->data + CHUNK_SIZE(c->status));
    return same_heap(c, next) ? next : NULL;
}

heap_chunk *prev_chunk(heap_chunk *c) {
    if (c->prev_size == 0)
        return NULL;
    return (heap_chunk *)((char *)c - c->prev_size);
}

void coalesce(allocator *a, heap_chunk *chunk) {
    uint64_t size = CHUNK_SIZE(chunk->status);
    debug_printf("coalesce: %p (size %lu)\n", chunk, size);

    heap_chunk *prev = prev_chunk(chunk);
    if (prev && IS_FREE(prev->status)) {
        a->free_chunk_tree = rb_tree_remove_node(a->free_chunk_tree, prev);
        size += CHUNK_SIZE(prev->status) + sizeof(heap_chunk);
        chunk = prev;
    }
    heap_chunk *next_absolute = next_chunk(chunk);
    if (next_absolute != NULL && next_absolute->status & FREE) {
        a->free_chunk_tree =
            rb_tree_remove_node(a->free_chunk_tree, next_absolute);
        size += CHUNK_SIZE(next_absolute->status) + sizeof(heap_chunk);
    } else {
        if (next_absolute != NULL)
            next_absolute->prev_size = size;
    }

    // maybe leak some memory
    size &= ~(CHUNK_SIZE_ALIGN - 1);
    free_chunk_init(chunk, size, chunk->prev_size);

    a->free_chunk_tree =
        rb_tree_insert(a->free_chunk_tree, (free_chunk_tree *)chunk);
    rb_tree_debug_print(a->free_chunk_tree);

    debug_printf("END coalesce: %p (size %lu)\n", chunk,
                 CHUNK_SIZE(chunk->status));
}

void *allo_cate_standard(allocator *a, size_t to_alloc) {
    debug_printf("allo_cate: standard %lu\n", to_alloc);
    free_chunk *best_fit = rb_tree_search(a->free_chunk_tree, to_alloc);

    if (best_fit == NULL) {
        best_fit = add_heap(a);
        if (best_fit == NULL)
            return NULL;
    } else {
        a->free_chunk_tree = rb_tree_remove_node(a->free_chunk_tree, best_fit);
    }

    best_fit->status &= ~FREE;

    // try split node
    if (CHUNK_SIZE(best_fit->status) - to_alloc >= sizeof(heap_chunk)) {
        size_t leftover = CHUNK_SIZE(best_fit->status) - to_alloc;
        size_t rounded_down = leftover & ~(CHUNK_SIZE_ALIGN - 1);

        if (rounded_down > MAX_ARENA_SIZE) {
            size_t new_size = CHUNK_SIZE(best_fit->status) - rounded_down;
            // should be aligned already but just in case
            new_size &= ~(CHUNK_SIZE_ALIGN - 1);

            debug_printf("Split node of size %lu into %lu and %lu\n",
                         CHUNK_SIZE(best_fit->status), new_size, rounded_down);

            free_chunk *split_chunk = (free_chunk *)(best_fit->data + new_size);
            free_chunk_init(split_chunk, rounded_down, new_size);

            coalesce(a, split_chunk);

            best_fit->status = new_size;
        }
    }

    a->stats.num_bytes_allocated += CHUNK_SIZE(best_fit->status);

    return best_fit->data;
}

void *allo_cate(allocator *a, size_t size) {
    debug_printf("allo_cate: %lu\n", size);
    rb_tree_debug_print(a->free_chunk_tree);

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
    rb_tree_debug_print(a->free_chunk_tree);

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
    heap_chunk *ch = (heap_chunk *)((char *)p - sizeof(heap_chunk));
    debug_printf("allo_free_standard: %lu\n", CHUNK_SIZE(ch->status));
    ch->status |= FREE;
    coalesce(a, ch);
}

void allo_free(allocator *a, void *p) {
    if (p == NULL)
        return;
    chunk *c = (chunk *)((char *)p - sizeof(chunk));

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
}

size_t introspect_size(void *p) {
    return CHUNK_SIZE(((chunk *)((char *)p - sizeof(chunk)))->status);
}

// malloc etc.
allocator global_allocator = (allocator){
    .heaps = NULL,
    .free_chunk_tree = NULL,
    .mmapped_chunk_head = NULL,
    .stats = {0},
    .arenas = {0},
};

void *_allo_malloc(size_t size) { return allo_cate(&global_allocator, size); }

void _allo_free(void *p) { allo_free(&global_allocator, p); }

void *_allo_realloc(void *p, size_t size) {
    if (p == NULL)
        return _allo_malloc(size);
    if (introspect_size(p) >= size)
        return p;
    void *new_p = malloc(size);
    memcpy(new_p, p, introspect_size(p));
    free(p);
    return new_p;
}

void *_allo_calloc(size_t nmemb, size_t size) {
    void *p = malloc(nmemb * size);
    memset(p, 0, nmemb * size);
    return p;
}
