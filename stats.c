#include "stats.h"

void initialize_stats(stats *s) {
    s->num_bytes_allocated = 0;
    s->total_heap_size     = 0;
}
