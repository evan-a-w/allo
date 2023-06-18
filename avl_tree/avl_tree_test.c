#include <stdio.h>
#include <assert.h>
#include "avl_tree.h"
#include "../allo.h"

#define TEST_SIZE 1000 // Test tree with 1000 nodes

void test_avl_tree_insert(void) {
    tree_node *root = NULL;
    tree_node nodes[TEST_SIZE];

    // Inserting and checking for presence
    for (unsigned i = 0; i < TEST_SIZE; i++) {
        nodes[i].status = i * 32u;
        nodes[i].child[0] = nodes[i].child[1] = NULL;
        nodes[i].parent = NULL;

        root = avl_tree_insert(root, &nodes[i]);
    }

    for (unsigned j = 0; j < TEST_SIZE; j++) {
        assert(avl_tree_search(root, j * 32) == (node *)&nodes[j]);
    }
}

void test_avl_tree_remove(void) {
    tree_node *root = NULL;
    tree_node nodes[TEST_SIZE];

    for (unsigned i = 0; i < TEST_SIZE; i++) {
        nodes[i].status = i * 32;
        nodes[i].child[0] = nodes[i].child[1] = NULL;
        nodes[i].parent = NULL;
        root = avl_tree_insert(root, &nodes[i]);
    }

    // Removing and checking for absence
    for (unsigned i = 0; i < TEST_SIZE; i++) {
        root = avl_tree_remove(root, i * 32);
        for (unsigned j = 0; j < i; j++) {
            node *search = avl_tree_search(root, j * 32);
            assert(search && CHUNK_SIZE(search->status) == j * 32);
        }
        node *n = avl_tree_search(root, i * 32);
        assert(n == NULL || CHUNK_SIZE(n->status) != i * 32);
    }
}

void test_avl_tree_remove_node(void) {
    tree_node *root = NULL;
    tree_node nodes[TEST_SIZE];

    for (unsigned i = 0; i < TEST_SIZE; i++) {
        nodes[i].status = i * 32;
        nodes[i].child[0] = nodes[i].child[1] = NULL;
        nodes[i].parent = NULL;
        root = avl_tree_insert(root, &nodes[i]);
    }

    // Removing and checking for absence
    for (unsigned i = 0; i < TEST_SIZE; i++) {
        root = avl_tree_remove_node(root, (node *)&nodes[i]);
        node *n = avl_tree_search(root, i * 32);
        assert(n == NULL || CHUNK_SIZE(n->status) != i * 32);
    }
}

int main(void) {
    test_avl_tree_insert();
    printf("Passed insert\n");
    test_avl_tree_remove();
    printf("Passed remove\n");
    test_avl_tree_remove_node();
    printf("Passed remove_node\n");
    printf("All tests passed!\n");
    return 0;
}
