#include "allo.h"
#include "rb_tree.h"
#include <stdio.h>

allocator a;

void allocate_large_strings(void) {
    initialize_allocator(&a);
    char *strings[10];
    for (int i = 0; i < 10; i++) {
        char *x = allo_cate(&a, 5000);
        snprintf(x, 100, "hello world %d", i);
        strings[i] = x;
    }

    for (int i = 0; i < 10; i++) {
        printf("%s\n", strings[i]);
    }

    rb_tree_debug_print(a.free_chunk_tree);

    free_allocator(&a);
}

int main(void) {
    allocate_large_strings();

    return 0;
}
