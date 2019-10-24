#include <iostream>
#include <cstring>
#include <vector>
#include "huffman.hpp"

struct args {
    bool is_verbose;
    bool compress;
    std::string in_file_name;
    std::string out_file_name;
};

args parse_args(int argc, char **argv) {
    if (argc < 4) {
        throw std::invalid_argument("usage: huffman [-v] (-c|-d) input_file output_file");
    }
    bool is_v = false;
    bool is_compress = false;
    bool is_decompress = false;
    std::string in_file_name = std::string(argv[argc - 2]);
    std::string out_file_name = std::string(argv[argc - 1]);

    for (int i = 1; i < argc - 2; ++i) {
        if (strcmp(argv[i], "-v") == 0) {
            is_v = true;
        }
        if (strcmp(argv[i], "-c") == 0) {
            if (is_decompress) {
                throw std::invalid_argument("-c -d options can't be used simultaneously");
            }
            is_compress = true;
        }
        if (strcmp(argv[i], "-d") == 0) {
            if (is_compress) {
                throw std::invalid_argument("-c -d options can't be used simultaneously");
            }
            is_decompress = true;
        }
    }

    if (!is_compress && !is_decompress) {
        throw std::invalid_argument("at least one of -c -d options should be passed");
    }

    return {is_v, is_compress, in_file_name, out_file_name};
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

int main(int argc, char **argv) {
    try {
        args arguments = parse_args(argc, argv);
        std::ifstream in_file(arguments.in_file_name, std::ios::binary);
        std::ofstream out_file(arguments.out_file_name, std::ios::binary);
        if (arguments.compress) {
            std::pair<uint64_t *, size_t> p = calc_frequencies(&in_file);
            auto frequencies = p.first;
            auto file_size = p.second;
            huff_tree *tree = build_tree(frequencies);
            if (tree == nullptr) {
                print_compression_statistics(compression_statistics{file_size, 0, 0});
                delete[] frequencies;
                delete tree;
                return 0;
            }
            std::vector<char> *codes = build_codes(tree);
            in_file.clear();
            in_file.seekg(0, std::ios::beg);
            compression_statistics statistics = compress(&in_file, &out_file, codes, frequencies);
            print_compression_statistics(statistics);
            std::vector<char> *v = new std::vector<char>();
            if (arguments.is_verbose) {
                print_codes(tree->root, v);
            }
            delete[] frequencies;
            delete tree;
            delete[] codes;
            delete v;
        } else {
            auto p = decompress(&in_file, &out_file);
            decompression_statistics statistics = p.first;
            huff_tree *tree = p.second;
            print_decompression_statistics(statistics);
            std::vector<char> *v = new std::vector<char>();
            if (arguments.is_verbose) {
                print_codes(tree->root, v);
            }
            delete v;
            delete tree;
        }

        return 0;
    } catch (std::exception &e) {
        std::cerr << e.what();
        return 1;
    }
}