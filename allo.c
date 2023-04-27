#include "allo.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE 4096

typedef struct page {
    struct page *next;
    size_t size;
    char data[];
} page;

typedef struct allocator {
    uint64_t num_bytes_allocated;
    uint64_t page_size;
    page *pages;
} allocator;

typedef struct free_chunk {
    size_t size;
    size_t prev_size;
    struct chunk *next_of_size;
} free_chunk;

typedef struct allocated_chunk {
    size_t size;
    char data[];
} allocated_chunk;

page *get_currpage(char *ptr) {
    return (page *) ((uint64_t)ptr & ~(PAGE_SIZE - 1));
}

void allocator_init(allocator *a, uint64_t page_size) {
    a->num_bytes_allocated = 0;
    a->page_size = page_size;
    a->pages = NULL;
}

int main(void) {
    printf("Hello, World!\n");
    return 0;
}
