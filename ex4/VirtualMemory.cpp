//TODO delete iostream
#include <iostream>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"




void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    clearTable(0);


}

int temp(uint64_t virtualAddress, int depth){
    return virtualAddress >> (2 - depth ) * OFFSET_WIDTH;
}

void readRec(uint64_t virtualAddress, word_t* value){
    int x = 1302;
    std::cout << temp(x, 0) ;
}

int VMread(uint64_t virtualAddress, word_t* value) {
    readRec(virtualAddress , value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    return 1;
}
