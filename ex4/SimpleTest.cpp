#include "VirtualMemory.h"
#include <iostream>
#include <cstdio>
#include <cassert>
int main(int argc, char **argv) {
    VMinitialize();
    int x = 13; // 01101
    int y;

    VMread(13 , &y);
//    std::cout << "************" << std::endl;
//    VMread(6 , &y);

//    std::cout << "this is for second " << y << std::endl;
//    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
//        printf("writing to %llu\n", (long long int) i);
//        VMwrite(5 * i * PAGE_SIZE, i);
//    }
//
//    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
//        word_t value;
//        VMread(5 * i * PAGE_SIZE, &value);
//        printf("reading from %llu %d\n", (long long int) i, value);
//        assert(uint64_t(value) == i);
//    }
//    printf("success\n");

    return 0;
}