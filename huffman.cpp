#include <fstream>
#include <queue>
#include <iostream>
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

struct huff_tree_node;

struct huff_tree {
    huff_tree_node *root;

    explicit huff_tree(huff_tree_node *root) {
        this->root = root;
    }

    ~huff_tree() {
        delete root;
    }
};

struct huff_tree_node {
    huff_tree_node *left;
    huff_tree_node *right;
    uint32_t frequency;
    uint8_t symbol;

    huff_tree_node(huff_tree_node *left, huff_tree_node *right, uint64_t frequency, uint8_t symbol) {
        this->left = left;
        this->right = right;
        this->frequency = frequency;
        this->symbol = symbol;
    }

    ~huff_tree_node() {
        delete this->left;
        delete this->right;
    }
};

struct compression_statistics {
    size_t compressed_size;
    size_t source_size;
    size_t additional_information_size;
};

struct decompression_statistics {
    size_t decompressed_size;
    size_t compressed_size;
    size_t additional_information_size;
};

struct decompression_result {
    decompression_statistics statistics;
    huff_tree *root;
};

std::pair<uint32_t *, size_t> calc_frequencies(std::ifstream *in_file) {
    auto *frequencies = new uint32_t[256]();
    size_t file_size = 0;

    while (true) {
        char c = in_file->get();
        if (in_file->eof()) {
            break;
        }
        file_size++;
        frequencies[static_cast<uint8_t>(c)]++;
    }

    return std::make_pair(frequencies, file_size);
}

huff_tree *build_tree(uint32_t *frequencies) {
    auto is_greater = [](const huff_tree_node *left, const huff_tree_node *right) {
        if (left->frequency == right->frequency) {
            return left->symbol < right->symbol;
        }

        return left->frequency < right->frequency;
    };
    std::priority_queue<huff_tree_node *, std::vector<huff_tree_node *>, decltype(is_greater)> Q(is_greater);
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] != 0) {
            Q.push(new huff_tree_node(nullptr, nullptr, frequencies[i], i));
        }
    }

    while (Q.size() > 1) {
        huff_tree_node *left = Q.top();
        Q.pop();
        huff_tree_node *right = Q.top();
        Q.pop();
        Q.push(new huff_tree_node(left, right, left->frequency + right->frequency, 0));
    }

    if (Q.empty()) {
        return nullptr;
    }
    auto *tree = new huff_tree(Q.top());
    return tree;
}

void traverse_tree(huff_tree_node *root, std::vector<char> *codes, std::vector<char> *prefix) {
    if (root == nullptr) {
        return;
    }

    if (root->left == nullptr && root->right == nullptr) {
        codes[root->symbol] = *prefix;
    }

    prefix->push_back(0);
    traverse_tree(root->left, codes, prefix);
    prefix->pop_back();
    prefix->push_back(1);
    traverse_tree(root->right, codes, prefix);
    prefix->pop_back();
}

std::vector<char> *build_codes(huff_tree *tree) {
    auto codes = new std::vector<char>[256];
    auto root = tree->root;
    if (root->left == nullptr && root->right == nullptr) {
        codes[root->symbol] = {0};
        return codes;
    }

    auto prefix = new std::vector<char>();
    traverse_tree(tree->root, codes, prefix);
    delete prefix;

    return codes;
}

compression_statistics
_compress(std::ifstream &in, std::ofstream &out, std::vector<char> *codes, uint32_t *frequencies) {
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
    out.write(reinterpret_cast<char *>(&file_len), 8);
    out.write(reinterpret_cast<char *>(&num_non_zero_frequencies), 8);
    overhead_size += 16;
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] == 0) {
            continue;
        }
        out.write(reinterpret_cast<char *>(&reinterpret_cast<uint8_t &>(i)), 1);
        out.write(reinterpret_cast<char *>(&frequencies[i]), 4);
        overhead_size += 5;
    }

    uint8_t cur_byte = 0;
    uint8_t num_bits = 0;

    while (!in.eof()) {
        char c = in.get();
        if (in.eof()) {
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
                out.write(reinterpret_cast<char *>(&cur_byte), 1);
                cur_byte = 0;
                num_bits = 0;
            }
        }
    }

    if (num_bits > 0) {
        compressed_size++;
        cur_byte <<= 8u - num_bits;
        out.write(reinterpret_cast<char *>(&cur_byte), 1);
    }

    return compression_statistics{compressed_size, source_size, overhead_size};
}

decompression_result _decompress(std::ifstream &in, std::ofstream &out) {
    size_t compressed_size = 0;
    size_t decompressed_size = 0;
    size_t overhead_size = 0;

    if (in.eof()) {
        return decompression_result{decompression_statistics{0, 0, 0}, nullptr};
    }
    char buf[8]{};
    in.read(buf, 8);
    if (in.eof()) {
        return decompression_result{decompression_statistics{0, 0, 0}, nullptr};
    }
    overhead_size += 8;
    uint64_t file_len = *reinterpret_cast<uint64_t *>(buf);
    in.read(buf, 8);
    overhead_size += 8;
    uint64_t num_non_zero_frequencies = *reinterpret_cast<uint64_t *>(buf);
    uint32_t frequencies[256]{};
    for (uint64_t i = 0; i < num_non_zero_frequencies; ++i) {
        in.read(buf, 1);
        uint8_t symbol = *reinterpret_cast<uint8_t *>(buf);
        in.read(buf, 4);
        uint32_t frequency = *reinterpret_cast<uint32_t *>(buf);
        frequencies[symbol] = frequency;
        overhead_size += 5;
    }

    huff_tree *tree = build_tree(frequencies);

    huff_tree_node *cur_node = tree->root;
    uint64_t num_symbols_decoded = 0;
    while (!in.eof()) {
        char c = in.get();
        uint8_t u = reverse(reinterpret_cast<uint8_t &>(c));
        compressed_size++;

        for (uint8_t i = 0; i < 8; ++i) {
            auto root = tree->root;
            if (root->left == nullptr && root->right == nullptr) {
                out.write(reinterpret_cast<char *>(&root->symbol), 1);
                decompressed_size++;
                cur_node = tree->root;
                num_symbols_decoded++;
                if (num_symbols_decoded == file_len) {
                    return decompression_result{
                            decompression_statistics{decompressed_size, compressed_size, overhead_size}, tree};
                }
                continue;
            }
            uint8_t cur_bit = u & 1;
            u >>= 1;
            if (cur_bit == 1) {
                if (cur_node != nullptr) {
                    cur_node = cur_node->right;
                }
            } else {
                if (cur_node != nullptr) {
                    cur_node = cur_node->left;
                }
            }
            if (cur_node->left == nullptr && cur_node->right == nullptr) {
                out.write(reinterpret_cast<char *>(&cur_node->symbol), 1);
                decompressed_size++;
                cur_node = tree->root;
                num_symbols_decoded++;
                if (num_symbols_decoded == file_len) {
                    return decompression_result{
                            decompression_statistics{decompressed_size, compressed_size, overhead_size}, tree};
                }
            }
        }
    }

    return decompression_result{decompression_statistics{decompressed_size, compressed_size, overhead_size}, tree};
}

void print_codes(huff_tree_node *node, std::vector<char> *cur_code, int depth) {
    if (node == nullptr) {
        return;
    }
    if (node->left == nullptr && node->right == nullptr) {
        uint8_t symbol = node->symbol;
        if (depth == 0) {
            std::cout << 0;
        }
        for (size_t i = 0; i < (*cur_code).size(); ++i) {
            std::cout << static_cast<uint16_t>((*cur_code)[i]);
        }
        std::cout << " ";
        std::cout << unsigned(symbol) << std::endl;
        return;
    }

    cur_code->push_back(0);
    print_codes(node->left, cur_code, depth + 1);
    cur_code->pop_back();
    cur_code->push_back(1);
    print_codes(node->right, cur_code, depth + 1);
    cur_code->pop_back();
}


void print_compression_statistics(compression_statistics statistics) {
    std::cout << statistics.source_size << std::endl;
    std::cout << statistics.compressed_size << std::endl;
    std::cout << statistics.additional_information_size << std::endl;
}

void print_decompression_statistics(decompression_statistics statistics) {
    std::cout << statistics.compressed_size << std::endl;
    std::cout << statistics.decompressed_size << std::endl;
    std::cout << statistics.additional_information_size << std::endl;
}

void decompress(std::ifstream &in_file, std::ofstream &out_file, bool is_verbose) {
    decompression_result result = _decompress(in_file, out_file);
    print_decompression_statistics(result.statistics);
    auto v = new std::vector<char>();
    if (is_verbose) {
        print_codes(result.root->root, v, 0);
    }
    delete v;
    delete result.root;
}

void compress(std::ifstream &in, std::ofstream &out, bool verbose) {
    std::pair<uint32_t *, size_t> p = calc_frequencies(&in);
    auto frequencies = p.first;
    auto file_size = p.second;
    huff_tree *tree = build_tree(frequencies);
    if (tree == nullptr) {
        print_compression_statistics(compression_statistics{file_size, 0, 0});
        delete[] frequencies;
        delete tree;
        return;
    }
    std::vector<char> *codes = build_codes(tree);
    in.clear();
    in.seekg(0, std::ios::beg);
    compression_statistics statistics = _compress(in, out, codes, frequencies);
    print_compression_statistics(statistics);
    auto v = new std::vector<char>();
    if (verbose) {
        print_codes(tree->root, v, 0);
    }
    delete[] frequencies;
    delete tree;
    delete[] codes;
    delete v;
}