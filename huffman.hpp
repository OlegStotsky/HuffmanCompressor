#pragma once

#include <cstddef>
#include <fstream>

struct huff_tree_node;

struct huff_tree {
    huff_tree_node *root;

    explicit huff_tree(huff_tree_node *root) {
        this->root = root;
    }

    ~huff_tree();
};

struct huff_tree_node {
    huff_tree_node *left;
    huff_tree_node *right;
    uint64_t frequency;

    huff_tree_node(huff_tree_node *left, huff_tree_node *right, uint64_t frequency) {
        this->left = left;
        this->right = right;
        this->frequency = frequency;
    }

    virtual ~huff_tree_node();
};

struct huff_tree_leaf_node : public huff_tree_node {
    uint8_t symbol;

    huff_tree_leaf_node(uint64_t frequency, uint8_t symbol) : huff_tree_node(nullptr, nullptr, frequency) {
        this->symbol = symbol;
    }
};

uint64_t *calc_frequencies(std::ifstream *in_file);

huff_tree *build_tree(uint64_t *frequencies);

std::vector<char> *build_codes(huff_tree *tree);

void compress(std::ifstream *in, std::ofstream *out, std::vector<char> *codes, uint64_t *frequencies);

void decompress(std::ifstream *in, std::ofstream *out);
