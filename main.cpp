#include <cstring>
#include <stdexcept>

struct args {
    bool is_verbose;
    bool compress;
    bool decompress;
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

    return {is_v, is_compress, is_decompress, in_file_name, out_file_name};
}

int main(int argc, char **argv) {
    try {
        args arguments = parse_args(argc, argv);
    } catch (std::invalid_argument e) {
        std::cerr << e.what();
    }

    return 0;
}
>>>>>>> initial commit
