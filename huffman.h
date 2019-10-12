#pragma once

#include <cstddef>
#include <fstream>

struct huff_tree_node;

struct huff_tree {
    huff_tree_node *root;

    explicit huff_tree(huff_tree_node *root) {
        this->root = root;
    }
};

struct huff_tree_node {
    huff_tree_node *left;
    huff_tree_node *right;
    int frequency;

    huff_tree_node(huff_tree_node *left, huff_tree_node *right, int frequency) {
        this->left = left;
        this->right = right;
        this->frequency = frequency;
    }

    virtual ~huff_tree_node() {};
};

struct huff_tree_leaf_node : public huff_tree_node {
    uint8_t symbol;

    huff_tree_leaf_node(int frequency, u_int8_t symbol) : huff_tree_node(nullptr, nullptr, frequency) {
        this->symbol = symbol;
    }
};

size_t *calc_frequencies(std::ifstream *in_file);

huff_tree *build_tree(size_t *frequencies);

uint8_t *build_codes(huff_tree *tree);