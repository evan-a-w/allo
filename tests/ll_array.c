#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_ALLOCATIONS 1000
#define MAX_ALLOC_SIZE 2048

// Custom malloc and free functions
void *my_malloc(size_t size);
void my_free(void *ptr);

int main() {
    srand(time(NULL));

    void *allocated_memory[NUM_ALLOCATIONS] = {NULL};

    for (int i = 0; i < NUM_ALLOCATIONS; i++) {
        size_t alloc_size = rand() % MAX_ALLOC_SIZE + 1;
        allocated_memory[i] = my_malloc(alloc_size);
        if (allocated_memory[i] == NULL) {
            printf("Failed to allocate memory at iteration %d\n", i);
            return 1;
        }
        memset(allocated_memory[i], 0, alloc_size);
    }

    // Free the allocated memory in a random order
    for (int i = 0; i < NUM_ALLOCATIONS; i++) {
        int index_to_free = rand() % NUM_ALLOCATIONS;
        while (allocated_memory[index_to_free] == NULL) {
            index_to_free = (index_to_free + 1) % NUM_ALLOCATIONS;
        }
        my_free(allocated_memory[index_to_free]);
        allocated_memory[index_to_free] = NULL;
    }

    printf("Test passed: All memory allocations and frees were successful.\n");
    return 0;
}

// Implement your custom malloc and free functions here
void *my_malloc(size_t size) { return malloc(size); }

void my_free(void *ptr) { free(ptr); }
