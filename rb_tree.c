#include "rb_tree.h"

#include "allo.h"

static inline int is_red(free_chunk_tree *node) {
    return node != NULL && IS_RED(node->status);
}

static inline void set_red(free_chunk_tree *node) { node->status |= RED; }

static free_chunk_tree *min(free_chunk_tree *node) {
    if (node == NULL)
        return NULL;
    while (node->left != NULL)
        node = node->left;
    return node;
}

static free_chunk_tree *rotate_left(free_chunk_tree *h) {
    free_chunk_tree *x = h->right;
    if (x == NULL)
        return h;
    h->right = x->left;
    x->left = h;
    x->status &= ~RED;
    x->status |= h->status & RED;
    set_red(h);
    return x;
}

static free_chunk_tree *rotate_right(free_chunk_tree *h) {
    free_chunk_tree *x = h->left;
    if (x == NULL)
        return h;
    h->left = x->right;
    x->right = h;
    x->status &= ~RED;
    x->status |= h->status & RED;
    set_red(h);
    return x;
}

static void flip_colors(free_chunk_tree *h) {
    if (h->left == NULL || h->right == NULL)
        return;
    h->status ^= RED;
    h->left->status ^= RED;
    h->right->status ^= RED;
}

static free_chunk_tree *move_red_left(free_chunk_tree *h) {
    flip_colors(h);
    if (h->right && is_red(h->right->left)) {
        h->right = rotate_right(h->right);
        h = rotate_left(h);
        flip_colors(h);
    }
    return h;
}

static free_chunk_tree *move_red_right(free_chunk_tree *h) {
    flip_colors(h);
    if (h->left && is_red(h->left->left)) {
        h = rotate_right(h);
        flip_colors(h);
    }
    return h;
}

static free_chunk_tree *balance(free_chunk_tree *h) {
    if (is_red(h->right) && !is_red(h->left))
        h = rotate_left(h);
    if (is_red(h->left) && is_red(h->left->left))
        h = rotate_right(h);
    if (is_red(h->left) && is_red(h->right))
        flip_colors(h);
    return h;
}

free_chunk *rb_tree_search(free_chunk_tree *root, size_t size) {
    while (root != NULL) {
        size_t chunk_size = CHUNK_SIZE(root->status);
        if (size == chunk_size) {
            return root->next_of_size ? (free_chunk *)root->next_of_size
                                      : (free_chunk *)root;
        } else if (size < chunk_size) {
            if (root->left == NULL)
                return root->next_of_size ? (free_chunk *)root->next_of_size
                                          : (free_chunk *)root;
            root = root->left;
        } else {
            root = root->right;
        }
    }
    return NULL;
}

free_chunk_tree *rb_tree_insert(free_chunk_tree *h, free_chunk_tree *new_node) {
    if (h == NULL) {
        new_node->status |= RED;
        new_node->status |= TREE;
        new_node->left = NULL;
        new_node->right = NULL;
        new_node->next_of_size = NULL;
        return new_node;
    }

    if (is_red(h->left) && is_red(h->right)) {
        flip_colors(h);
    }

    size_t chunk_size = CHUNK_SIZE(h->status);
    size_t new_chunk_size = CHUNK_SIZE(new_node->status);

    if (new_chunk_size == chunk_size) {
        free_chunk_list *new_list = (free_chunk_list *)new_node;
        new_list->next_of_size = h->next_of_size;
        new_list->prev_of_size = (free_chunk *)h;
        new_list->status &= ~TREE;
        if (h->next_of_size != NULL)
            h->next_of_size->prev_of_size = (free_chunk *)new_list;
        h->next_of_size = new_list;
    } else if (new_chunk_size < chunk_size) {
        h->left = rb_tree_insert(h->left, new_node);
    } else {
        h->right = rb_tree_insert(h->right, new_node);
    }

    if (is_red(h->right) && !is_red(h->left))
        h = rotate_left(h);
    if (is_red(h->left) && is_red(h->left->left))
        h = rotate_right(h);
    if (is_red(h->left) && is_red(h->right))
        flip_colors(h);

    return h;
}

void print_free_chunk_list(free_chunk_list *list) {
    debug_printf("[");
    while (list != NULL) {
        debug_printf("(%zu, %p)", CHUNK_SIZE(list->status), (void *)list);
        list = list->next_of_size;
        if (list != NULL) {
            debug_printf(", ");
        }
    }
    debug_printf("]");
}

void print_rb_tree_helper(free_chunk_tree *node, int level) {
    if (node == NULL) {
        return;
    }

    print_rb_tree_helper(node->right, level + 1);

    for (int i = 0; i < level; ++i) {
        debug_printf("  ");
    }

    debug_printf("%zu%s: ", CHUNK_SIZE(node->status), is_red(node) ? "R" : "B");
    print_free_chunk_list((free_chunk_list *)node);
    debug_printf("\n");

    print_rb_tree_helper(node->left, level + 1);
}

void rb_tree_debug_print(free_chunk_tree *root) {
    print_rb_tree_helper(root, 0);
}

static free_chunk_tree *delete_min(free_chunk_tree *h) {
    if (h == NULL)
        return NULL;

    if (h->left == NULL)
        return NULL;

    if (h->left && !is_red(h->left) && !is_red(h->left->left))
        h = move_red_left(h);

    h->left = delete_min(h->left);
    return balance(h);
}

free_chunk_tree *rb_tree_remove(free_chunk_tree *h, size_t size) {
    if (h == NULL)
        return NULL;
    size_t chunk_size = CHUNK_SIZE(h->status);

    if (size < chunk_size) {
        if (h->left && !is_red(h->left) && !is_red(h->left->left))
            h = move_red_left(h);
        h->left = rb_tree_remove(h->left, size);
    } else {
        if (!is_red(h->left))
            h = rotate_right(h);
        if (size == chunk_size && h->right == NULL) {
            return NULL;
        }
        if (h->right && !is_red(h->right) && !is_red(h->right->left))
            h = move_red_right(h);
        if (size == chunk_size) {
            free_chunk_tree *x = min(h->right);
            h->right = delete_min(h->right);
            x->left = h->left;
            x->right = h->right;
            if (is_red(h))
                x->status |= RED;
            else
                x->status &= ~RED;
            h = x;
        } else {
            h->right = rb_tree_remove(h->right, size);
        }
    }

    return balance(h);
}

free_chunk_tree *rb_tree_remove_one(free_chunk_tree *h, size_t size,
                                    free_chunk **removed_node) {
    if (h == NULL) {
        if (removed_node != NULL)
            *removed_node = NULL;
        return NULL;
    }

    size_t chunk_size = CHUNK_SIZE(h->status);

    if (size == chunk_size && h->next_of_size != NULL) {
        free_chunk_list *list = h->next_of_size;
        if (removed_node)
            *removed_node = (free_chunk *)list;
        if (list->next_of_size != NULL)
            list->next_of_size->prev_of_size = (free_chunk *)h;
        h->next_of_size = list->next_of_size;
        return h;
    }

    if (size < chunk_size) {
        if (h->left && !is_red(h->left) && !is_red(h->left->left))
            h = move_red_left(h);
        h->left = rb_tree_remove_one(h->left, size, removed_node);
    } else {
        if (!is_red(h->left))
            h = rotate_right(h);
        if (size == chunk_size && h->right == NULL) {
            if (removed_node)
                *removed_node = (free_chunk *)h;
            return NULL;
        }
        if (h->right && !is_red(h->right) && !is_red(h->right->left))
            h = move_red_right(h);
        if (size == chunk_size) {
            if (removed_node)
                *removed_node = (free_chunk *)h;
            free_chunk_tree *x = min(h->right);
            h->right = delete_min(h->right);
            x->left = h->left;
            x->right = h->right;
            if (is_red(h))
                x->status |= RED;
            else
                x->status &= ~RED;
            h = x;
        } else {
            h->right = rb_tree_remove_one(h->right, size, removed_node);
        }
    }

    return balance(h);
}

free_chunk_tree *rb_tree_remove_node(free_chunk_tree *h, free_chunk *node) {
    if (IS_TREE(node->status))
        return rb_tree_remove(h, CHUNK_SIZE(node->status));

    // otherwise remove from list
    free_chunk_list *list = (free_chunk_list *)node;
    ((free_chunk_list *)list->prev_of_size)->next_of_size = list->next_of_size;

    if (list->next_of_size != NULL)
        ((free_chunk_list *)list->next_of_size)->prev_of_size =
            list->prev_of_size;

    return h;
}
