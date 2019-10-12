#include <fstream>
#include <queue>
#include "huffman.h"


size_t *calc_frequencies(std::ifstream *in_file) {
    size_t *frequencies = new size_t[256]();

    while (!in_file->eof()) {
        char c = in_file->get();
        frequencies[reinterpret_cast<uint8_t &>(c)]++;
    }

    return frequencies;
}

huff_tree *build_tree(size_t *frequencies) {
    auto is_greater = [](huff_tree_node *left, huff_tree_node *right) { return left->frequency > right->frequency; };
    std::priority_queue<huff_tree_node *, std::vector<huff_tree_node *>, decltype(is_greater)> Q(is_greater);
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] == 0) {
            continue;
        }
        Q.push(new huff_tree_leaf_node(frequencies[i], i));
    }

    while (Q.size() > 1) {
        huff_tree_node *left = Q.top();
        Q.pop();
        huff_tree_node *right = Q.top();
        Q.pop();
        Q.push(new huff_tree_node(left, right, left->frequency + right->frequency));
    }

    huff_tree *tree = new huff_tree(Q.top());
    return tree;
}

void traverse_tree(huff_tree_node *root, uint8_t *codes, std::bitset<8> *prefix, int depth) {
    if (root == nullptr) {
        return;
    }

    huff_tree_leaf_node *as_leaf = dynamic_cast<huff_tree_leaf_node *>(root);
    if (as_leaf != nullptr) {
        uint8_t code = prefix->to_ulong();
        codes[as_leaf->symbol] = code;
    }

    prefix->set(depth, false);
    traverse_tree(root->left, codes, prefix, depth + 1);
    prefix->set(depth, true);
    traverse_tree(root->right, codes, prefix, depth + 1);
    prefix->set(depth, false);
}

uint8_t *build_codes(huff_tree *tree) {
    uint8_t *codes = new uint8_t[256]();
    traverse_tree(tree->root, codes, new std::bitset<8>(), 0);

    return codes;
}
