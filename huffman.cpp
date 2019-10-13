#include <fstream>
#include <queue>
#include "huffman.h"
#include "utils.h"


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

void traverse_tree(huff_tree_node *root, std::vector<char> *codes, std::vector<char> *prefix, int depth) {
    if (root == nullptr) {
        return;
    }

    huff_tree_leaf_node *as_leaf = dynamic_cast<huff_tree_leaf_node *>(root);
    if (as_leaf != nullptr) {
        codes[as_leaf->symbol] = *prefix;
    }

    prefix->push_back('0');
    traverse_tree(root->left, codes, prefix, depth + 1);
    prefix->pop_back();
    prefix->push_back('1');
    traverse_tree(root->right, codes, prefix, depth + 1);
    prefix->pop_back();
}

std::vector<char> *build_codes(huff_tree *tree) {
    std::vector<char> *codes = new std::vector<char>[256];
    traverse_tree(tree->root, codes, new std::vector<char>(), 0);

    return codes;
}

void compress(std::ifstream *in, std::ofstream *out, std::vector<char> *codes, size_t *frequencies) {
    uint8_t num_non_zero_frequencies = 0;
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            ++num_non_zero_frequencies;
        }
    }
    out->write((char *) &num_non_zero_frequencies, 1);
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] == 0) {
            continue;
        }
        out->write((char *) &reinterpret_cast<uint8_t &>(i), 1);
        out->write((char *) &frequencies[i], 1);
    }

    uint8_t cur_byte = 0;
    uint8_t num_bits = 0;

    while (!in->eof()) {
        char c = in->get();
        uint8_t u = reinterpret_cast<uint8_t &>(c);
        std::vector<char> &code = codes[u];
        for (int i = 0; i < code.size(); ++i) {
            cur_byte <<= 1;
            cur_byte |= char_to_uint8_t(code[i]);
            ++num_bits;
            if (num_bits == 8) {
                out->write((char *) &cur_byte, 1);
                cur_byte = 0;
                num_bits = 0;
            }
        }
    }

    if (num_bits > 0) {
        cur_byte <<= 8 - num_bits;
        (*out) << cur_byte;
    }
}

void decompress(std::ifstream *in, std::ofstream *out) {
    char c = in->get();
    uint8_t num_non_zero_frequencies = reinterpret_cast<uint8_t &>(c);
    size_t *frequencies = new size_t[256]();
    for (uint8_t i = 0; i < num_non_zero_frequencies; ++i) {
        char c = in->get();
        uint8_t symbol = reinterpret_cast<uint8_t &>(c);
        c = in->get();
        uint8_t frequency = reinterpret_cast<uint8_t &>(c);
        frequencies[symbol] = frequency;
    }

    huff_tree *tree = build_tree(frequencies);

    uint8_t cur_code = 0;
    huff_tree_node *cur_node = tree->root;
    while (!in->eof()) {
        char c = in->get();
        uint8_t u = reverse(reinterpret_cast<uint8_t &>(c));

        if (u == 0) {
            out->write((char *) &u, 1);
            continue;
        }

        for (uint8_t i = 0; i < 8; ++i) {
            uint8_t cur_bit = u & 1;
            u >>= 1;
            if (cur_bit == 1) {
                cur_node = cur_node->right;
            } else {
                cur_node = cur_node->left;
            }
            huff_tree_leaf_node *as_leaf = dynamic_cast<huff_tree_leaf_node *>(cur_node);
            if (as_leaf != nullptr) {
                out->write((char *) &as_leaf->symbol, 1);
                cur_node = tree->root;
            }
        }
    }
}