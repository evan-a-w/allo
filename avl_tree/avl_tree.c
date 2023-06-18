#include "avl_tree.h"

#include <assert.h>

#include "../allo.h"

#define LEFT 0
#define RIGHT 1

#define left child[LEFT]
#define right child[RIGHT]

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline size_t size(free_chunk_tree *node) {
    return CHUNK_SIZE(node->status);
}

static inline size_t height(free_chunk_tree *node) {
    return node ? node->height : 0;
}

static inline void update_height(free_chunk_tree *node) {
    node->height =
        1 + MAX(height(node->child[LEFT]), height(node->child[RIGHT]));
}

free_chunk_tree *rotate_dir(free_chunk_tree *P, int dir) {
    free_chunk_tree *G = P->parent;
    free_chunk_tree *S = P->child[1 - dir];
    if (!S) {
        return P;
    }
    free_chunk_tree *C = S->child[dir];
    P->child[1 - dir] = C;
    if (C != NULL)
        C->parent = P;
    S->child[dir] = P;
    P->parent = S;
    S->parent = G;
    if (G != NULL) {
        G->child[P == G->child[RIGHT] ? RIGHT : LEFT] = S;
        update_height(G);
    }
    update_height(P);
    update_height(C);
    return S;
}

#define rotate_left(P) rotate_dir(P, LEFT)
#define rotate_right(P) rotate_dir(P, RIGHT)
#define rotate_left_opt(P)                                                     \
    do {                                                                       \
        P &&rotate_dir(P, LEFT)                                                \
    } while (0)
#define rotate_right_opt(P)                                                    \
    do {                                                                       \
        P &&rotate_dir(P, RIGHT)                                               \
    } while (0)

free_chunk_tree *_avl_tree_search(free_chunk_tree *root, size_t size) {
    free_chunk_tree *node = root;
    while (node != NULL) {
        if ((size < CHUNK_SIZE(node->status) && node->child[LEFT] == NULL)
            || size == CHUNK_SIZE(node->status))
            return node;
        else
            node = node->child[size > CHUNK_SIZE(node->status)];
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
    node->status |= TREE;
    node->parent = NULL;
    node->next_of_size = NULL;
    node->child[LEFT] = NULL;
    node->child[RIGHT] = NULL;
    return node;
}

int64_t bf(free_chunk_tree *h) {
    return (int64_t)height(h->child[LEFT]) - (int64_t)height(h->child[RIGHT]);
}

free_chunk_tree *avl_tree_insert(free_chunk_tree *h,
                                 free_chunk_tree *new_node) {
    if (h == NULL) {
        return init(new_node);
    }

    if (CHUNK_SIZE(h->status) == CHUNK_SIZE(new_node->status)) {
        new_node->status &= ~TREE;
        new_node->next_of_size = h->next_of_size;
        h->next_of_size->prev_of_size = (free_chunk *)new_node;
        free_chunk_list *l = (free_chunk_list *)new_node;
        l->prev_of_size = (free_chunk *)h;
        h->next_of_size = l;
        return h;
    } else if (CHUNK_SIZE(h->status) > CHUNK_SIZE(new_node->status)) {
        h->child[LEFT] = avl_tree_insert(h->child[LEFT], new_node);
        h->child[LEFT]->parent = h;
    } else {
        h->child[RIGHT] = avl_tree_insert(h->child[RIGHT], new_node);
        h->child[RIGHT]->parent = h;
    }
    update_height(h);
    int64_t balance = bf(h);
    if (balance > 1) {
        if (h->left && CHUNK_SIZE(h->status) > CHUNK_SIZE(h->left->status)) {
            h->left = rotate_left(h->left);
        }
        h = rotate_right(h);
    } else if (balance < -1) {
        if (h->right && CHUNK_SIZE(h->status) < CHUNK_SIZE(h->right->status)) {
            h->right = rotate_right(h->left);
        }
        h = rotate_left(h);
    }
    update_height(h);
    return h;
}

free_chunk_tree *min_elt(free_chunk_tree *h) {
    if (h == NULL)
        return NULL;
    while (h->left != NULL)
        h = h->left;
    return h;
}

typedef enum { _LEFT, _RIGHT, _NEITHER } which_child;

which_child find_which_child(free_chunk_tree *parent, free_chunk_tree *child) {
    if (parent->left == child)
        return _LEFT;
    else if (parent->right == child)
        return _RIGHT;
    else
        return _NEITHER;
}

void full_switch(free_chunk_tree *a, free_chunk_tree *b) {
    free_chunk_tree *p = a->parent;
    free_chunk_tree *l = a->left;
    free_chunk_tree *r = a->right;
    a->parent = b->parent;
    a->left = b->left;
    a->right = b->right;
    b->parent = p;
    b->left = l;
    b->right = r;
    if (l)
        l->parent = b;
    if (r)
        r->parent = b;
    if (p) {
        if (p->left == a)
            p->left = b;
        else
            p->right = b;
    }
}

free_chunk_tree *avl_tree_remove(free_chunk_tree *h, size_t size) {
    if (h == NULL) {
        return NULL;
    }

    free_chunk_tree *res;
    if (CHUNK_SIZE(h->status) == size) {
        if (!h->left && !h->right) {
            res = NULL;
        } else if (h->left && !h->right) {
            h->left->parent = h->parent;
            res = h->left;
        } else if (h->right && !h->left) {
            h->right->parent = h->parent;
            res = h->right;
        } else {
            free_chunk_tree *min_node = min_elt(h->right);
            full_switch(h, min_node);
            res = min_node;
        }
    }
}

free_chunk_tree *avl_tree_remove_node(free_chunk_tree *h, free_chunk *node) {
    if (IS_TREE(node->status))
        return avl_tree_remove(h, CHUNK_SIZE(node->status));
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

    for (int i = 0; i < level; ++i) {
        debug_printf("  ");
    }

    debug_printf("%zu l: %p, r: %p, p: %p: ", CHUNK_SIZE(node->status),
                 (void *)node->child[LEFT], (void *)node->child[RIGHT],
                 node->parent);
    print_free_chunk_list((free_chunk_list *)node);
    debug_printf("\n");

    print_avl_tree_helper(node->child[LEFT], level + 1);
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
