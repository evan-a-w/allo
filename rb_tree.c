#include "allo.h"

#include "rb_tree.h"

static free_chunk_tree *fix_up(free_chunk_tree *h);
static free_chunk_tree *move_red_left(free_chunk_tree *h);

static inline int is_red(free_chunk_tree *node) {
    return node != NULL && IS_RED(node->status);
}

static inline void set_red(free_chunk_tree *node) { node->status |= RED; }

static inline void set_black(free_chunk_tree *node) { node->status &= ~RED; }

static free_chunk_tree *rotate_left(free_chunk_tree *h) {
    free_chunk_tree *x = h->right;
    h->right = x->left;
    x->left = h;
    x->status = h->status;
    set_red(h);
    return x;
}

static free_chunk_tree *rotate_right(free_chunk_tree *h) {
    free_chunk_tree *x = h->left;
    h->left = x->right;
    x->right = h;
    x->status = h->status;
    set_red(h);
    return x;
}

static void flip_colors(free_chunk_tree *h) {
    set_red(h);
    set_black(h->left);
    set_black(h->right);
}

free_chunk_tree *rb_tree_search(free_chunk_tree *root, size_t size) {
    while (root != NULL) {
        size_t chunk_size = CHUNK_SIZE(root->status);
        if (size == chunk_size) {
            return root;
        } else if (size < chunk_size) {
            if (root->right == NULL)
                return root;
            root = root->left;
        } else {
            root = root->right;
        }
    }
    return NULL;
}

free_chunk_tree *rb_tree_insert(free_chunk_tree *h, free_chunk_tree *new_node) {
    if (h == NULL) {
        new_node-> status |= TREE;
        return new_node;
    }

    if (is_red(h->left) && is_red(h->right)) {
        flip_colors(h);
    }

    size_t chunk_size = CHUNK_SIZE(h->status);
    size_t new_chunk_size = CHUNK_SIZE(new_node->status);

    if (new_chunk_size < chunk_size) {
        h->left = rb_tree_insert(h->left, new_node);
    } else if (new_chunk_size > chunk_size) {
        h->right = rb_tree_insert(h->right, new_node);
    } else {
        free_chunk_list *new_list = (free_chunk_list *)h;
        new_list->status &= ~TREE;
        new_list->next_of_size = h->next_of_size;
        if (h->next_of_size != NULL) {
            h->next_of_size->prev_of_size = (free_chunk *)new_node;
        }
        new_list->prev_of_size = (free_chunk *)h;
        h->next_of_size = new_list;
        return h;
    }

    if (is_red(h->right) && !is_red(h->left)) {
        h = rotate_left(h);
    }

    if (is_red(h->left) && is_red(h->left->left)) {
        h = rotate_right(h);
    }

    return h;
}

// Helper function to get the minimum node of the tree
static free_chunk_tree *min_node(free_chunk_tree *h) {
    while (h->left != NULL) {
        h = h->left;
    }
    return h;
}

// Helper function to delete the minimum node of the tree
static free_chunk_tree *delete_min(free_chunk_tree *h) {
    if (h->left == NULL) {
        return NULL;
    }

    if (!is_red(h->left) && !is_red(h->left->left)) {
        h = move_red_left(h);
    }

    h->left = delete_min(h->left);

    return fix_up(h);
}

static free_chunk_tree *move_red_left(free_chunk_tree *h) {
    flip_colors(h);
    if (is_red(h->right->left)) {
        h->right = rotate_right(h->right);
        h = rotate_left(h);
        flip_colors(h);
    }
    return h;
}

static free_chunk_tree *move_red_right(free_chunk_tree *h) {
    flip_colors(h);
    if (is_red(h->left->left)) {
        h = rotate_right(h);
        flip_colors(h);
    }
    return h;
}

static free_chunk_tree *fix_up(free_chunk_tree *h) {
    if (is_red(h->right)) {
        h = rotate_left(h);
    }

    if (is_red(h->left) && is_red(h->left->left)) {
        h = rotate_right(h);
    }

    if (is_red(h->left) && is_red(h->right)) {
        flip_colors(h);
    }

    return h;
}

free_chunk_tree *rb_tree_remove(free_chunk_tree *h, size_t size) {
    if (size < CHUNK_SIZE(h->status)) {
        if (!is_red(h->left) && !is_red(h->left->left)) {
            h = move_red_left(h);
        }
        h->left = rb_tree_remove(h->left, size);
    } else {
        if (is_red(h->left)) {
            h = rotate_right(h);
        }
        if (size == CHUNK_SIZE(h->status) && (h->right == NULL)) {
            return NULL;
        }
        if (!is_red(h->right) && !is_red(h->right->left)) {
            h = move_red_right(h);
        }
        if (size == CHUNK_SIZE(h->status)) {
            free_chunk_tree *min_node_right = min_node(h->right);
            h->status = min_node_right->status;
            h->right = delete_min(h->right);
        } else {
            h->right = rb_tree_remove(h->right, size);
        }
    }
    return fix_up(h);
}