#include "../include/concurrency_grep.h"

int main(int argc, char** argv) {
    // 1. Check arguments
    if (argc != 3) {
        fprintf(stderr, "usage: my_grep [pattern] [file]\n");
        exit(1);
    }

    // 2. Run grep
    if (-1 == grep_init(argv)) {
        exit(1);
    }

    return 0;
}
