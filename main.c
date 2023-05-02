#include "allo.h"
#include "rb_tree.h"
#include <stdio.h>

allocator a;

void allocate_strings(size_t size, void (*free_fn)(allocator *, void *)) {
    initialize_allocator(&a);
    char *strings[10];
    for (int i = 0; i < 10; i++) {
        char *x = allo_cate(&a, size);
        heap_chunk *chunk = (heap_chunk *)(x - sizeof(heap_chunk));
        printf("Got node of size: %zu\n", CHUNK_SIZE(chunk->status));
        snprintf(x, size, "hello world %d", i);
        strings[i] = x;
    }

    for (int i = 0; i < 10; i++) {
        printf("%s\n", strings[i]);
        free_fn(&a, strings[i]);
    }

    rb_tree_debug_print(a.free_chunk_tree);

    free_allocator(&a);
}

void noop(allocator *a, void *p) {
    (void)a;
    (void)p;
    return;
}

int main(void) {
    /* allocate_strings(1032, allo_free); */
    allocate_strings(1032, noop);

    return 0;
}
