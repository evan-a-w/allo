#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

const uint32_t CODE_MAX = (1u << 15) - 1;

typedef struct trie_node {
    struct trie_node *parent;
    struct trie_node *children[128];
    uint32_t code;
    char c;
} trie_node;

static trie_node dict = {};
static uint32_t curr_code = 0;

void print_trie_char(FILE *output_file, trie_node *node) {
    if (node == NULL)
        return;
    print_trie_char(output_file, node->parent);
    fputc(node->c, output_file);
}

#ifdef DEBUG
void print_trie_node(trie_node *node) {
    print_trie_char(stdout, node);
    printf(": %d\n", node->code);
}
#endif

trie_node *singleton(int c) {
    trie_node *child = dict.children[c];
    if (child == NULL) {
        child = dict.children[c] = calloc(1, sizeof(trie_node));
        child->parent = NULL;
        child->c = c;
        child->code = (1u << 16);
    }
    return child;
}

trie_node *extend(trie_node *curr, int c) {
    trie_node *child = curr->children[c];
    if (curr_code > CODE_MAX)
        return child;
    if (child == NULL) {
        child = curr->children[c] = calloc(1, sizeof(trie_node));
        child->parent = curr;
        child->c = c;
        child->code = curr_code++;
#ifdef DEBUG
        print_trie_node(child);
#endif
        return NULL;
    }
    return child;
}

void free_trie(trie_node *root) {
    if (root == NULL)
        return;
    for (int i = 0; i < 128; i++)
        free_trie(root->children[i]);
    free(root);
}

void output_code(FILE *output_file, uint32_t code) {
    uint8_t arr[2];
    assert(code <= CODE_MAX);
    arr[0] = (code >> 8u) | (1u << 7u);
    arr[1] = code & 0xFF;
    fwrite(arr, sizeof(uint8_t), 2, output_file);
}

bool should_print_code(trie_node *node) {
    return node->parent && node->parent->parent && node->code <= CODE_MAX;
}

void output_node(FILE *output_file, trie_node *node) {
    if (should_print_code(node))
        output_code(output_file, node->code);
    else
        print_trie_char(output_file, node);
}

void encode(FILE *input_file, FILE *output_file) {
    int c;
    trie_node *prev_node = NULL;

    while ((c = fgetc(input_file)) != EOF) {
        assert(c >= 0 && c < 128);
        trie_node *pc = prev_node ? extend(prev_node, c) : singleton(c);
        if (pc) {
            prev_node = pc;
        } else {
            if (prev_node)
                output_node(output_file, prev_node);
            prev_node = singleton(c);
        }
    }

    if (prev_node)
        output_node(output_file, prev_node);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
        return 1;
    }

    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        perror(argv[1]);
        return 1;
    }
    FILE *output_file = fopen(argv[2], "w");
    if (input_file == NULL) {
        perror(argv[2]);
        return 1;
    }

    encode(input_file, output_file);

    fclose(input_file);
    fclose(output_file);

    for (int i = 0; i < 128; i++)
        free_trie(dict.children[i]);

    return 0;
}
