#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "../allo.h"

const uint32_t CODE_MAX = (1u << 15) - 1;

/* The majority of the trie node code is duplicated across both of the files
 * because we couldn't use helper files */
typedef struct trie_node {
    struct trie_node *parent;
    struct trie_node *root;
    struct trie_node *children[128];
    uint32_t code;
    char c;
} trie_node;

static trie_node dict = {};
static uint32_t curr_code = 0;
static trie_node *map[1 << 15] = {};

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
        child->root = child;
        child->c = c;
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
        child->root = curr->root;
        child->c = c;
        child->code = curr_code++;
        map[child->code] = child;
#ifdef DEBUG
        print_trie_node(child);
#endif
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

bool is_index(int c) { return c & (1u << 7); }

struct input {
    enum { CHAR, CODE } type;
    int c;
};

bool read_input(FILE *input_file, struct input *inp) {
    int c = fgetc(input_file);
    if (c == EOF)
        return false;
    if (is_index(c)) {
        inp->type = CODE;
        inp->c = (c & 0x7f) << 8;
        c = fgetc(input_file);
        if (c == EOF)
            return false;
        inp->c |= c;
    } else {
        inp->type = CHAR;
        inp->c = c;
    }
    return true;
}

void decompress(FILE *input_file, FILE *output_file) {
    struct input inp;
    if (!read_input(input_file, &inp))
        return;
    if (inp.type == CODE) {
        fprintf(stderr, "First input must be a character\n");
        exit(1);
    }

    fputc(inp.c, output_file);
    trie_node *prev = singleton(inp.c);

    while (read_input(input_file, &inp)) {
        trie_node *curr;
        switch (inp.type) {
        case CHAR:
            if (prev->children[inp.c] != NULL) {
                fputc(inp.c, output_file);
                prev = prev->children[inp.c];
                continue;
            }
            curr = singleton(inp.c);
            break;
        case CODE:
            curr = map[inp.c];
            break;
        }

        if (curr == NULL) {
            curr = extend(prev, prev->root->c);
            if (curr == NULL) {
                fprintf(stderr, "Dictionary overflow\n");
                exit(1);
            }
            print_trie_char(output_file, curr);
        } else {
            extend(prev, curr->root->c);
            print_trie_char(output_file, curr);
        }
        prev = curr;
    }
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

    decompress(input_file, output_file);

    fclose(input_file);
    fclose(output_file);

    for (int i = 0; i < 128; i++)
        free_trie(dict.children[i]);

    return 0;
}
