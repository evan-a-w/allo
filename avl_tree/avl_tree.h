#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <stddef.h>
#include <stdint.h>

#include "../allo.h"

#define ALLO_AVL_DEBUG

typedef free_chunk node;
typedef free_chunk_tree tree_node;
typedef free_chunk_list list_node;

node *avl_tree_search(tree_node *root, size_t size);
tree_node *avl_tree_remove(tree_node *h, size_t size);
tree_node *avl_tree_insert(tree_node *h, tree_node *new_node);
tree_node *avl_tree_remove_node(tree_node *h, node *node);
void avl_tree_debug_print(tree_node *root);

#endif
