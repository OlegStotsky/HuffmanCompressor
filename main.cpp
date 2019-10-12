#include <cstring>
#include <stdexcept>

struct args {
    bool is_verbose;
    bool compress;
    bool decompress;
};

args parse_args(int argc, char **argv) {
    bool is_v = false;
    bool is_compress = false;
    bool is_decompress = false;

    for (int i = 0; i < argc; ++i) {
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

    return {is_v, is_compress, is_decompress};
}

int main(int argc, char **argv) {
    using namespace std;

    try {
        args arguments = parse_args(argc, argv);
    } catch (std::invalid_argument e) {
        cerr << e.what();
    }

    return 0;
}
>>>>>>> initial commit
