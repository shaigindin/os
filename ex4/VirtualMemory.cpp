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

uint64_t AdressOffset(uint64_t virtualAddress, int depth){
    return virtualAddress >> (TABLES_DEPTH - depth) * OFFSET_WIDTH;
}

uint64_t AddressSlicer(uint64_t addr, int depth)
{
    return (addr % (1 << ((TABLES_DEPTH - depth) * OFFSET_WIDTH)));
}

void readRec(uint64_t virtualAddress, word_t* value, int depth, word_t* addr){
    if (depth == TABLES_DEPTH)
    {
        // TODO: read
    }
    if (*addr != 0)
    {
        uint64_t firstAddr = AdressOffset(virtualAddress, depth);
        PMread((*addr) * PAGE_SIZE + firstAddr, addr);
        virtualAddress = AddressSlicer(virtualAddress, depth); // slicing the addr
        depth++;
    }

}



int VMread(uint64_t virtualAddress, word_t* value) {
    word_t addr;
    uint64_t firstAddr = AdressOffset(virtualAddress, 0);
    PMread(0 + firstAddr, &addr);
    virtualAddress = AddressSlicer(virtualAddress, 0); // slicing the addr
    int depth = 1;
    readRec(virtualAddress , value, depth, &addr);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    return 1;
}
