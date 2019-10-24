#include <fstream>
#include <queue>
#include <iostream>
#include "huffman.hpp"

namespace {
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

std::pair<uint64_t *, size_t> calc_frequencies(std::ifstream *in_file) {
    uint64_t *frequencies = new uint64_t[256]();
    size_t file_size = 0;

    while (true) {
        char c = in_file->get();
        if (in_file->eof()) {
            break;
        }
        file_size++;
        frequencies[reinterpret_cast<uint8_t &>(c)]++;
    }

    return std::make_pair(frequencies, file_size);
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
        codes[as_leaf->symbol] = {0};
        return codes;
    }
    traverse_tree(tree->root, codes, new std::vector<char>(), 0);

    return codes;
}

compression_statistics
compress(std::ifstream *in, std::ofstream *out, std::vector<char> *codes, uint64_t *frequencies) {
    size_t source_size = 0;
    size_t compressed_size = 0;
    size_t overhead_size = 0;
    uint64_t num_non_zero_frequencies = 0;
    uint64_t file_len = 0;
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            ++num_non_zero_frequencies;
            file_len += frequencies[i];
        }
    }
    out->write(reinterpret_cast<char *>(&file_len), 8);
    out->write(reinterpret_cast<char *>(&num_non_zero_frequencies), 8);
    overhead_size += 16;
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] == 0) {
            continue;
        }
        out->write(reinterpret_cast<char *>(&reinterpret_cast<uint8_t &>(i)), 1);
        out->write(reinterpret_cast<char *>(&frequencies[i]), 8);
        overhead_size += 9;
    }

    uint8_t cur_byte = 0;
    uint8_t num_bits = 0;

    while (!in->eof()) {
        char c = in->get();
        if (in->eof()) {
            break;
        }
        source_size++;
        uint8_t u = reinterpret_cast<uint8_t &>(c);
        std::vector<char> &code = codes[u];
        for (uint64_t i = 0; i < code.size(); ++i) {
            cur_byte <<= 1u;
            cur_byte |= code[i];
            ++num_bits;
            if (num_bits == 8) {
                compressed_size++;
                out->write((char *) &cur_byte, 1);
                cur_byte = 0;
                num_bits = 0;
            }
        }
    }

    if (num_bits > 0) {
        compressed_size++;
        cur_byte <<= 8u - num_bits;
        out->write((char *) &cur_byte, 1);
    }

    return compression_statistics{compressed_size, source_size, overhead_size};
}

std::pair<decompression_statistics, huff_tree *> decompress(std::ifstream *in, std::ofstream *out) {
    size_t compressed_size = 0;
    size_t decompressed_size = 0;
    size_t overhead_size = 0;

    if (in->eof()) {
        return std::make_pair(decompression_statistics{decompressed_size, compressed_size, overhead_size}, nullptr);
    }
    char buf[8]{};
    in->read(buf, 8);
    overhead_size += 8;
    if (in->eof()) {
        return std::make_pair(decompression_statistics{decompressed_size, compressed_size, overhead_size}, nullptr);
    }
    uint64_t file_len = *reinterpret_cast<uint64_t *>(buf);
    in->read(buf, 8);
    overhead_size += 8;
    uint64_t num_non_zero_frequencies = *reinterpret_cast<uint64_t *>(buf);
    uint64_t frequencies[256]{};
    for (uint64_t i = 0; i < num_non_zero_frequencies; ++i) {
        in->read(buf, 1);
        uint8_t symbol = *reinterpret_cast<uint8_t *>(buf);
        in->read(buf, 8);
        uint64_t frequency = *reinterpret_cast<uint64_t *>(buf);
        frequencies[symbol] = frequency;
        overhead_size += 9;
    }

    huff_tree *tree = build_tree(frequencies);

    huff_tree_node *cur_node = tree->root;
    uint64_t num_symbols_decoded = 0;
    while (!in->eof()) {
        char c = in->get();
        uint8_t u = reverse(reinterpret_cast<uint8_t &>(c));
        compressed_size++;

        for (uint8_t i = 0; i < 8; ++i) {
            huff_tree_leaf_node *root_as_leaf = dynamic_cast<huff_tree_leaf_node *>(tree->root);
            if (root_as_leaf != nullptr) {
                out->write(reinterpret_cast<char *>(&root_as_leaf->symbol), 1);
                decompressed_size++;
                cur_node = tree->root;
                num_symbols_decoded++;
                if (num_symbols_decoded == file_len) {
                    return std::make_pair(decompression_statistics{decompressed_size, compressed_size, overhead_size},
                                          tree);
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
                out->write(reinterpret_cast<char *>(&as_leaf->symbol), 1);
                decompressed_size++;
                cur_node = tree->root;
                num_symbols_decoded++;
                if (num_symbols_decoded == file_len) {
                    return std::make_pair(decompression_statistics{decompressed_size, compressed_size, overhead_size},
                                          tree);
                }
            }
        }
    }

    return std::make_pair(decompression_statistics{decompressed_size, compressed_size, overhead_size},
                          tree);
}

void print_codes(huff_tree_node *node, std::vector<char> *cur_code) {
    if (node == nullptr) {
        return;
    }
    huff_tree_leaf_node *as_leaf = dynamic_cast<huff_tree_leaf_node *>(node);
    if (as_leaf != nullptr) {
        char symbol = as_leaf->symbol;
        for (size_t i = 0; i < (*cur_code).size(); ++i) {
            std::cout << static_cast<uint16_t>((*cur_code)[i]) << " ";
        }
        std::cout << static_cast<uint16_t>(symbol) << std::endl;
        return;
    }

    cur_code->push_back(0);
    print_codes(node->left, cur_code);
    cur_code->pop_back();
    cur_code->push_back(1);
    print_codes(node->right, cur_code);
    cur_code->pop_back();
}
