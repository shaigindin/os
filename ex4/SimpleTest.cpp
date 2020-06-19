#include "VirtualMemory.h"

#include <cstdio>
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    VMinitialize();
    std::cout << VIRTUAL_MEMORY_SIZE << std::endl;
    for (uint64_t i = 0; i < (VIRTUAL_MEMORY_SIZE); ++i) {
        printf("writing to %llu\n", (long long int) i);
        VMwrite(i, i);
    }

    for (uint64_t i = 0; i <  (VIRTUAL_MEMORY_SIZE); ++i) {
        word_t value;
        VMread(i, &value);
        printf("reading from %llu %d\n", (long long int) i, value);
        assert(uint64_t(value) == i);
    }
    printf("success\n");

    return 0;
}