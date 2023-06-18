#include "rb_tree.h"

#include <assert.h>

#include "allo.h"

#define LEFT 0
#define RIGHT 1

static inline size_t size(free_chunk_tree *node) {
    return CHUNK_SIZE(node->status);
}

static inline int is_red(free_chunk_tree *node) {
    return node != NULL && IS_RED(node->status);
}

static inline void set_red(free_chunk_tree *node) { node->status |= RED; }

static free_chunk_tree *rotate_dir(free_chunk_tree *P, int dir) {
    free_chunk_tree *G = P->parent;
    free_chunk_tree *S = P->child[1 - dir];
    assert(S != NULL); // pointer to true node required
    free_chunk_tree *C = S->child[dir];
    P->child[1 - dir] = C;
    if (C != NULL)
        C->parent = P;
    S->child[dir] = P;
    P->parent = S;
    S->parent = G;
    if (G != NULL)
        G->child[P == G->child[RIGHT] ? RIGHT : LEFT] = S;
    return S; // new root of subtree
}

#define rotate_left(P) rotate_dir(P, LEFT)
#define rotate_right(P) rotate_dir(P, RIGHT)

free_chunk_tree *_rb_tree_search(free_chunk_tree *root, size_t size) {
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

free_chunk *rb_tree_search(free_chunk_tree *root, size_t size) {
    free_chunk_tree *node = _rb_tree_search(root, size);
    return (node && node->next_of_size) ? (free_chunk *)node->next_of_size
                                        : (free_chunk *)node;
}

free_chunk_tree *rb_tree_insert(free_chunk_tree *h, free_chunk_tree *new_node) {
    if (h == NULL) {
        new_node->parent = NULL;
        new_node->child[LEFT] = NULL;
        new_node->child[RIGHT] = NULL;
        new_node->next_of_size = NULL;
        return new_node;
    }

    if (CHUNK_SIZE(h->status) == CHUNK_SIZE(new_node->status)) {
        free_chunk_list *list = (free_chunk_list *)new_node;
        list->next_of_size = h->next_of_size;
        list->prev_of_size = (free_chunk *)h;
        h->next_of_size = list;
        if (list->next_of_size != NULL)
            list->next_of_size->prev_of_size = (free_chunk *)list;
        new_node->status &= ~TREE;
        return h;
    }

    int dir = CHUNK_SIZE(h->status) < CHUNK_SIZE(new_node->status);

    h->child[dir] = rb_tree_insert(h->child[dir], new_node);
    if (h->child[dir] == new_node) {
        new_node->parent = h;
        new_node->status |= TREE;
    }

    return h;
}

free_chunk_tree *rb_tree_remove(free_chunk_tree *h, size_t size) {
    free_chunk_tree *node = _rb_tree_search(h, size);
    if (node == NULL || CHUNK_SIZE(node->status) != size)
        return h;

    free_chunk_tree *replace_node = NULL;
    if (node->child[LEFT] && node->child[RIGHT]) {
        free_chunk_tree *min_right = node->child[RIGHT];
        while (min_right->child[LEFT] != NULL)
            min_right = min_right->child[LEFT];
        free_chunk_tree *max_min_right = min_right;
        while (max_min_right->child[RIGHT] != NULL)
            max_min_right = max_min_right->child[RIGHT];
        max_min_right->child[RIGHT] =
            rb_tree_remove(node->child[RIGHT], CHUNK_SIZE(min_right->status));
        min_right->child[LEFT] = node->child[LEFT];
        replace_node = min_right;

        if (min_right->child[LEFT])
            min_right->child[LEFT]->parent = min_right;
        if (min_right->child[RIGHT])
            min_right->child[RIGHT]->parent = min_right;
    } else if (node->child[LEFT]) {
        node->child[LEFT]->parent = node->parent;
        replace_node = node->child[LEFT];
    } else if (node->child[RIGHT]) {
        node->child[RIGHT]->parent = node->parent;
        replace_node = node->child[RIGHT];
    }

    if (node->parent) {
        if (node->parent->child[LEFT] == node)
            node->parent->child[LEFT] = replace_node;
        else
            node->parent->child[RIGHT] = replace_node;
    }

    return replace_node;
}

free_chunk_tree *rb_tree_remove_node(free_chunk_tree *h, free_chunk *node) {
    if (IS_TREE(node->status))
        return rb_tree_remove(h, CHUNK_SIZE(node->status));
    free_chunk_list *list = (free_chunk_list *)node;
    if (list->next_of_size)
        list->next_of_size->prev_of_size = list->prev_of_size;
    ((free_chunk_list *)list->prev_of_size)->next_of_size = list->next_of_size;
    return h;
}

static void print_free_chunk_list(free_chunk_list *list) {
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

void print_rb_tree_helper(free_chunk_tree *node, int level) {
    if (node == NULL)
        return;

    print_rb_tree_helper(node->child[RIGHT], level + 1);

    for (int i = 0; i < level; ++i) {
        debug_printf("  ");
    }

    debug_printf("%zu l: %p, r: %p, p: %p: ", CHUNK_SIZE(node->status),
                 (void *)node->child[LEFT], (void *)node->child[RIGHT],
                 node->parent);
    print_free_chunk_list((free_chunk_list *)node);
    debug_printf("\n");

    print_rb_tree_helper(node->child[LEFT], level + 1);
}

void rb_tree_debug_print(free_chunk_tree *root) {
#ifdef ALLO_RB_DEBUG
    debug_printf("RB Tree:\n");
    print_rb_tree_helper(root, 0);
    debug_printf("END RB Tree:\n");
#else
    (void)root;
#endif
}
