#include <fstream>
#include <queue>
#include "huffman.hpp"

uint8_t reverse(uint8_t x) {
    uint8_t result = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        uint8_t cur_bit = x & 1;
        x >>= 1;
        for (uint8_t j = 0; j < 8 - i - 1; ++j) {
            cur_bit <<= 1;
        }
        result |= cur_bit;
    }

    return result;
}


huff_tree::~huff_tree() {
    if (this->root != nullptr) {
        delete root;
    }
}

huff_tree_node::~huff_tree_node() {
    if (this->left != nullptr) {
        delete this->left;
    }
    if (this->right != nullptr) {
        delete this->right;
    }
}

uint64_t *calc_frequencies(std::ifstream *in_file) {
    uint64_t *frequencies = new uint64_t[256]();

    while (true) {
        char c = in_file->get();
        if (in_file->eof()) {
            break;
        }
        frequencies[reinterpret_cast<uint8_t &>(c)]++;
    }

    return frequencies;
}

huff_tree *build_tree(uint64_t *frequencies) {
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

    if (Q.size() == 0) {
        return nullptr;
    }
    huff_tree *tree = new huff_tree(Q.top());
    return tree;
}

void traverse_tree(huff_tree_node *root, std::vector<char> *codes, std::vector<char> *prefix, uint64_t depth) {
    if (root == nullptr) {
        return;
    }

    huff_tree_leaf_node *as_leaf = dynamic_cast<huff_tree_leaf_node *>(root);
    if (as_leaf != nullptr) {
        codes[as_leaf->symbol] = *prefix;
    }

    prefix->push_back(0);
    traverse_tree(root->left, codes, prefix, depth + 1);
    prefix->pop_back();
    prefix->push_back(1);
    traverse_tree(root->right, codes, prefix, depth + 1);
    prefix->pop_back();
}

std::vector<char> *build_codes(huff_tree *tree) {
    std::vector<char> *codes = new std::vector<char>[256];
    huff_tree_leaf_node *as_leaf = dynamic_cast<huff_tree_leaf_node *>(tree->root);
    if (as_leaf != nullptr) {
        codes[as_leaf->symbol] = {'0'};
        return codes;
    }
    traverse_tree(tree->root, codes, new std::vector<char>(), 0);

    return codes;
}

void compress(std::ifstream *in, std::ofstream *out, std::vector<char> *codes, uint64_t *frequencies) {
    uint64_t num_non_zero_frequencies = 0;
    uint64_t file_len = 0;
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            ++num_non_zero_frequencies;
            file_len += frequencies[i];
        }
    }
    out->write((char *) &file_len, 8);
    out->write((char *) &num_non_zero_frequencies, 8);
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] == 0) {
            continue;
        }
        out->write((char *) &reinterpret_cast<uint8_t &>(i), 1);
        out->write((char *) &frequencies[i], 8);
    }

    uint8_t cur_byte = 0;
    uint8_t num_bits = 0;

    while (!in->eof()) {
        char c = in->get();
        if (in->eof()) {
            break;
        }
        uint8_t u = reinterpret_cast<uint8_t &>(c);
        std::vector<char> &code = codes[u];
        for (uint64_t i = 0; i < code.size(); ++i) {
            cur_byte <<= 1u;
            cur_byte |= code[i];
            ++num_bits;
            if (num_bits == 8) {
                out->write((char *) &cur_byte, 1);
                cur_byte = 0;
                num_bits = 0;
            }
        }
    }

    if (num_bits > 0) {
        cur_byte <<= 8u - num_bits;
        out->write((char *) &cur_byte, 1);
    }
}

void decompress(std::ifstream *in, std::ofstream *out) {
    if (in->eof()) {
        return;
    }
    char buf[8]{};
    in->read(buf, 8);
    if (in->eof()) {
        return;
    }
    uint64_t file_len = *reinterpret_cast<uint64_t *>(buf);
    in->read(buf, 8);
    uint64_t num_non_zero_frequencies = *reinterpret_cast<uint64_t *>(buf);
    uint64_t frequencies[256]{};
    for (uint64_t i = 0; i < num_non_zero_frequencies; ++i) {
        in->read(buf, 1);
        uint8_t symbol = *reinterpret_cast<uint8_t *>(buf);
        in->read(buf, 8);
        uint64_t frequency = *reinterpret_cast<uint64_t *>(buf);
        frequencies[symbol] = frequency;
    }

    huff_tree *tree = build_tree(frequencies);

    huff_tree_node *cur_node = tree->root;
    uint64_t num_symbols_decoded = 0;
    while (!in->eof()) {
        char c = in->get();
        uint8_t u = reverse(reinterpret_cast<uint8_t &>(c));

        for (uint8_t i = 0; i < 8; ++i) {
            huff_tree_leaf_node *root_as_leaf = dynamic_cast<huff_tree_leaf_node *>(tree->root);
            if (root_as_leaf != nullptr) {
                out->write((char *) &root_as_leaf->symbol, 1);
                cur_node = tree->root;
                num_symbols_decoded++;
                if (num_symbols_decoded == file_len) {
                    return;
                }
                continue;
            }
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
                num_symbols_decoded++;
                if (num_symbols_decoded == file_len) {
                    return;
                }
            }
        }
    }
}