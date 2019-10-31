#include <iostream>
#include <cstring>
#include <vector>
#include <variant>
#include "huffman.hpp"

struct args {
    bool is_verbose;
    bool compress;
    std::string in_file_name;
    std::string out_file_name;
};

std::variant<std::string, args> parse_args(int argc, char **argv) {
    if (argc < 4) {
        return "usage: huffman [-v] (-c|-d) input_file output_file";
    }
    bool is_v = false;
    bool is_compress = false;
    bool is_decompress = false;
    std::string in_file_name = std::string(argv[argc - 2]);
    std::string out_file_name = std::string(argv[argc - 1]);

    for (int i = 1; i < argc - 2; ++i) {
        if (std::string(argv[i]) == "-v") {
            is_v = true;
        } else if (std::string(argv[i]) == "-c") {
            if (is_decompress) {
                return "-c -d options can't be used simultaneously";
            }
            is_compress = true;
        } else if (std::string(argv[i]) == "-d") {
            if (is_compress) {
                return "-c -d options can't be used simultaneously";
            }
            is_decompress = true;
        } else {
            return "unknown option: " + std::string(argv[i]);
        }
    }

    if (!is_compress && !is_decompress) {
        return "at least one of -c -d options should be passed";
    }

    return args{is_v, is_compress, in_file_name, out_file_name};
}

int main(int argc, char **argv) {
    auto parse_result = parse_args(argc, argv);
    if (const auto error = std::get_if<0>(&parse_result)) {
        std::cerr << *error;
        return 1;
    }
    args arguments = std::get<args>(parse_result);
    std::ifstream in_file(arguments.in_file_name, std::ios::binary);
    std::ofstream out_file(arguments.out_file_name, std::ios::binary);
    if (arguments.compress) {
        compress(in_file, out_file, arguments.is_verbose);
    } else {
        decompress(in_file, out_file, arguments.is_verbose);
    }

    return 0;
}