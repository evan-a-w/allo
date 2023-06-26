#include "avl_tree.h"

#include <assert.h>
#include <stdbool.h>

#include "../allo.h"

#define LEFT 0
#define RIGHT 1

#define left child[LEFT]
#define right child[RIGHT]

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline int64_t height(free_chunk_tree *node) {
    return node ? (int64_t)node->height : -1;
}

static inline void update_height(free_chunk_tree *node) {
    node->height =
        1 + MAX(height(node->child[LEFT]), height(node->child[RIGHT]));
}

free_chunk_tree *rotate_dir(free_chunk_tree *root, int dir) {
    free_chunk_tree *pivot = root->child[1 - dir];
    if (!pivot)
        return root;
    root->child[1 - dir] = pivot->child[dir];
    pivot->child[dir] = root;
    update_height(pivot);
    update_height(root);
    return pivot;
}

#define rotate_left(P) (rotate_dir(P, LEFT))
#define rotate_right(P) (rotate_dir(P, RIGHT))
#define rotate_left_opt(P) (P ? rotate_left(P) : NULL)
#define rotate_right_opt(P) (P ? rotate_right(P) : NULL)

free_chunk_tree *_avl_tree_search(free_chunk_tree *root, size_t size) {
    free_chunk_tree *node = root;
    while (node != NULL) {
        if ((size < SIZE(node) && node->child[LEFT] == NULL)
            || size == SIZE(node))
            return node;
        else
            node = node->child[size > SIZE(node)];
    }
    return NULL;
}

free_chunk *avl_tree_search(free_chunk_tree *root, size_t size) {
    free_chunk_tree *node = _avl_tree_search(root, size);
    return (node && node->next_of_size) ? (free_chunk *)node->next_of_size
                                        : (free_chunk *)node;
}

free_chunk_tree *init(free_chunk_tree *node) {
    node->height = 1;
    node->status |= TREE | FREE;
    node->next_of_size = NULL;
    node->child[LEFT] = NULL;
    node->child[RIGHT] = NULL;
    return node;
}

int64_t bf(free_chunk_tree *h) {
    return height(h->child[LEFT]) - height(h->child[RIGHT]);
}

free_chunk_tree *rebalance(free_chunk_tree *h) {
    if (!h)
        return NULL;
    update_height(h);
    int64_t balance = bf(h);
    if (balance > 1) {
        if (h->child[LEFT] && bf(h->child[LEFT]) < 0)
            h->child[LEFT] = rotate_left(h->child[LEFT]);
        return rotate_right(h);
    } else if (balance < -1) {
        if (h->child[RIGHT] && bf(h->child[RIGHT]) > 0)
            h->child[RIGHT] = rotate_right(h->child[RIGHT]);
        return rotate_left(h);
    } else {
        return h;
    }
}

free_chunk_tree *avl_tree_insert(free_chunk_tree *h,
                                 free_chunk_tree *new_node) {
    if (h == NULL)
        return init(new_node);
    int compare = SSIZE(new_node) - SSIZE(h);
    if (compare < 0) {
        if (h->child[LEFT] == NULL) {
            h->child[LEFT] = init(new_node);
        } else {
            h->child[LEFT] = avl_tree_insert(h->child[LEFT], new_node);
        }
    } else if (compare > 0) {
        if (h->child[RIGHT] == NULL) {
            h->child[RIGHT] = init(new_node);
        } else {
            h->child[RIGHT] = avl_tree_insert(h->child[RIGHT], new_node);
        }
    } else {
        free_chunk_list *l = h->next_of_size;

        free_chunk_list *new_list = (free_chunk_list *)init(new_node);
        new_list->status &= ~TREE;
        h->next_of_size = new_list;
        new_list->next_of_size = l;
        new_list->prev_of_size = (free_chunk *)h;
        if (l)
            l->prev_of_size = (free_chunk *)new_list;
    }
    return rebalance(h);
}

free_chunk_tree *min_elt(free_chunk_tree *h, free_chunk_tree *parent,
                         free_chunk_tree **save_parent) {
    if (h == NULL)
        return NULL;
    while (h->left != NULL) {
        parent = h;
        h = h->left;
    }
    *save_parent = parent;
    return h;
}

void full_switch(free_chunk_tree *a, free_chunk_tree *b) {
    uint64_t height = a->height;
    free_chunk_tree *l = a->left;
    free_chunk_tree *r = a->right;
    a->left = b->left;
    a->right = b->right;
    a->height = b->height;
    b->height = height;
    b->left = l;
    b->right = r;
}

free_chunk_tree *avl_tree_remove(free_chunk_tree *h, size_t size) {
    if (h == NULL) {
        return NULL;
    }

    int64_t compare = size - SSIZE(h);
    if (compare == 0) {
        if (!h->left && !h->right) {
            return NULL;
        } else if (h->left && !h->right) {
            return h->left;
        } else if (!h->left && h->right) {
            return h->right;
        } else {
            free_chunk_tree *parent = NULL;
            free_chunk_tree *min = min_elt(h->right, h, &parent);
            if (parent != h)
                parent->left = h;
            free_chunk_tree *tmp = h->right;
            h->right = min->right;
            if (min != tmp)
                min->right = tmp;
            else
                min->right = NULL;
            min->left = h->left;
            h->left = NULL;
            min->right = avl_tree_remove(min->right, SIZE(h));
            return rebalance(min);
        }
    } else if (compare < 0) {
        if (!h->left)
            return h;
        h->left = avl_tree_remove(h->left, size);
        return rebalance(h);
    } else {
        if (!h->right)
            return h;
        h->right = avl_tree_remove(h->right, size);
        return rebalance(h);
    }
}

free_chunk_tree *avl_tree_remove_node(free_chunk_tree *h, free_chunk *node) {
    if (IS_TREE(node->status))
        return avl_tree_remove(h, SIZE(node));
    free_chunk_list *list = (free_chunk_list *)node;
    if (list->next_of_size)
        list->next_of_size->prev_of_size = list->prev_of_size;
    ((free_chunk_list *)list->prev_of_size)->next_of_size = list->next_of_size;
    return h;
}

void print_free_chunk_list(free_chunk_list *list) {
    debug_printf("[");
    int num = 0;
    while (list != NULL) {
        debug_printf("(%zu, %p)", CHUNK_SIZE(list->status), (void *)list);
        list = list->next_of_size;
        if (list != NULL) {
            debug_printf(", ");
        }
        num++;
        if (num > 10) {
            debug_printf("...");
            assert(0);
            break;
        }
    }
    debug_printf("]");
}

void print_avl_tree_helper(free_chunk_tree *node, int level) {
    if (node == NULL)
        return;

    print_avl_tree_helper(node->child[RIGHT], level + 1);

    for (int i = 0; i < level * 2; ++i) {
        debug_printf("  ");
    }

    debug_printf("%zu (%p) l: %p, r: %p", CHUNK_SIZE(node->status),
                 (void *)node, (void *)node->child[LEFT],
                 (void *)node->child[RIGHT]);
    print_free_chunk_list(node->next_of_size);
    debug_printf("\n");

    print_avl_tree_helper(node->child[LEFT], level + 1);
}

bool avl_tree_contains(tree_node *root, node *node) {
    if (root == NULL)
        return false;
    if ((void *)root == (void *)node)
        return true;
    for (free_chunk_list *l = root->next_of_size; l != NULL;
         l = l->next_of_size) {
        if ((void *)l == (void *)node)
            return true;
    }
    return avl_tree_contains(root->left, node)
           || avl_tree_contains(root->right, node);
}

void avl_tree_debug_print(free_chunk_tree *root) {
#ifdef ALLO_AVL_DEBUG
    debug_printf("RB Tree:\n");
    print_avl_tree_helper(root, 0);
    debug_printf("END RB Tree:\n");
#else
    (void)root;
#endif
}

#undef left
#undef right
