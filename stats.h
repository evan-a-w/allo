#ifndef STATS_H
#define STATS_H

#include <stdint.h>

typedef struct stats {
    uint64_t num_bytes_allocated;
    uint64_t total_heap_size;
} stats;

void initialize_stats(stats *s);

#endif
