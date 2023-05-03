#include <stdio.h>

#include "allo.h"
#include "rb_tree.h"

allocator a;

void allocate_strings(size_t size, void (*free_fn)(allocator *, void *)) {
    initialize_allocator(&a);
    char *strings[10];
    for (int i = 0; i < 10; i++) {
        char *x = allo_cate(&a, size);
        snprintf(x, size, "%d hello world %d", i, i);
        strings[i] = x;
    }

    for (int i = 0; i < 10; i++) {
        printf("%s\n", strings[i]);
        free_fn(&a, strings[i]);
    }

    rb_tree_debug_print(a.free_chunk_tree);

    free_allocator(&a);
}

void allocate_strings_free(size_t size) {
    initialize_allocator(&a);

    for (int i = 0; i < 10; i++) {
        char *x = allo_cate(&a, size);
        snprintf(x, size, "%d hello world %d", i, i);
        rb_tree_debug_print(a.free_chunk_tree);
        printf("%s\n", x);
        rb_tree_debug_print(a.free_chunk_tree);
        allo_free(&a, x);
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
    /* allocate_strings(1032, noop); */
    allocate_strings_free(1032);

    return 0;
}
