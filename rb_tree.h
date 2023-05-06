#ifndef RB_TREE_H
#define RB_TREE_H

#include <stddef.h>
#include <stdint.h>

#include "allo.h"

/* #define ALLO_RB_DEBUG */

free_chunk *rb_tree_search(free_chunk_tree *root, size_t size);
free_chunk_tree *rb_tree_remove(free_chunk_tree *h, size_t size);
free_chunk_tree *rb_tree_remove_one(free_chunk_tree *h, size_t size,
                                    free_chunk **removed_node);
free_chunk_tree *rb_tree_insert(free_chunk_tree *h, free_chunk_tree *new_node);
free_chunk_tree *rb_tree_remove_node(free_chunk_tree *h, free_chunk *node);
void rb_tree_debug_print(free_chunk_tree *root);

#endif
