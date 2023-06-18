#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "allo.h"

#define NUM_ALLOCATIONS 5
#define MAX_ALLOC_SIZE 512

void test(void) {
    /* time_t t = time(NULL); */
    /* FILE *fp = fopen("./.seed", "w"); */
    /* fprintf(fp, "%ld\n", t); */
    /* fclose(fp); */
    /* srand(t); */

    srand(1684345974);

    int *allocated_memory[NUM_ALLOCATIONS] = {NULL};
    int sz[NUM_ALLOCATIONS] = {0};

    for (int i = 0; i < NUM_ALLOCATIONS; i++) {
        sz[i] = (rand() % MAX_ALLOC_SIZE + 1);

        allocated_memory[i] = malloc(sz[i] * 4);
        assert(allocated_memory[i] != NULL);
        for (int j = 0; j < sz[i]; j++)
            allocated_memory[i][j] = i;
    }

    printf("---------------- Done allocating --------------\n");

    // Free the allocated memory in a random order
    for (int i = 0; i < NUM_ALLOCATIONS; i++) {
        int index_to_free = rand() % NUM_ALLOCATIONS;
        if (allocated_memory[index_to_free] == NULL)
            continue;
        for (int j = 0; j < sz[index_to_free]; j++)
            assert(allocated_memory[index_to_free][j] == index_to_free);
        free(allocated_memory[index_to_free]);
        allocated_memory[index_to_free] = NULL;
    }

    for (int i = 0; i < NUM_ALLOCATIONS; i++) {
        if (allocated_memory[i] == NULL)
            continue;
        for (int j = 0; j < sz[i]; j++)
            assert(allocated_memory[i][j] == i);
        free(allocated_memory[i]);
        allocated_memory[i] = NULL;
    }

    printf("FREEING ALLOCATOR\n");
    free_allocator(&global_allocator);

    printf("Test passed: All memory allocations, data integrity checks, and "
           "frees were successful.\n");
}

int main(void) {
    test();

    return 0;
}
