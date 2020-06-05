//TODO: (1) delete iostream
//TODO: (2) delete the ram_insert function from the header file and from the .cpp
#include <iostream>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"


static int max_frame_index;

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    clearTable(0);
    max_frame_index = 1;
//    ram_insert(0, 0, 0);
//    ram_insert(0, 1, 0);
//    //
//    ram_insert(1, 0, 0);
//    ram_insert(1, 1, 0);
//    //
//    ram_insert(2, 0, 0);
//    ram_insert(2, 1, 0);
//    //
//    ram_insert(3, 0, 0);
//    ram_insert(3, 1, 0);
//    //
    ram_insert(4, 0, 0);
    ram_insert(4, 1, 1337);
    PMevict(4, 6);
}

int DfsFindBlank(uint64_t* frame_number)
{
    // TODO: implement
}

uint64_t fetchBlock()
{
    if (max_frame_index < NUM_FRAMES)
    {
        return max_frame_index++;
    }
    uint64_t frame_number;
    if (DfsFindBlank(&frame_number)) // We've found a block with zeros
    {
        return frame_number;
    }
}

uint64_t AdressOffset(uint64_t virtualAddress, int depth){
    return virtualAddress >> (TABLES_DEPTH - depth) * OFFSET_WIDTH;
}

uint64_t AddressSlicer(uint64_t addr, int depth)
{
    return (addr % (1 << ((TABLES_DEPTH - depth) * OFFSET_WIDTH)));
}

uint64_t getPageRoute(uint64_t page)
{
    return (page >> OFFSET_WIDTH);
}

/**
 *
 * @param virtualAddress The rest of the address to process.
 * @param value The end result to read (The value in the actual RAM).
 * @param depth Recursion depth.
 * @param next_address The pointer to the next frame.
 * @param current_address  The pointer from the current frame.
 */
void readRec(uint64_t virtualAddress, word_t* value, int depth,
        word_t* next_address, word_t current_address, uint64_t restoredFrameIndex){
    if (*next_address == 0)
    {
        uint64_t unused_frame = fetchBlock();
        clearTable(unused_frame);
        if (TABLES_DEPTH == depth)
        {
            PMrestore(unused_frame, restoredFrameIndex);
            uint64_t offset = AdressOffset(virtualAddress, depth);
            PMread(unused_frame * PAGE_SIZE + offset, next_address);
            std::cout << *next_address << std::endl;
            *value = *next_address;
            return;
        }
        uint64_t offset = AdressOffset(virtualAddress, depth - 1);
        PMwrite((current_address) * PAGE_SIZE + offset, unused_frame);
        *next_address = unused_frame;
        readRec(virtualAddress, value, depth,
                next_address, current_address, restoredFrameIndex);
    }
    else if (TABLES_DEPTH == depth)
    {
        uint64_t offset = AdressOffset(virtualAddress, depth);
        PMread((*next_address) * PAGE_SIZE + offset, next_address);
        std::cout << *next_address << std::endl;
        *value = *next_address;
        return;
    }
    else if (*next_address != 0)
    {
        std::cout << *next_address << std::endl;
        uint64_t offset = AdressOffset(virtualAddress, depth);
        current_address = *next_address;
        PMread((*next_address) * PAGE_SIZE + offset, next_address);
        virtualAddress = AddressSlicer(virtualAddress, depth); // slicing the next_address
        depth++;
        readRec(virtualAddress, value, depth,
                next_address, current_address, restoredFrameIndex);
    }
}



int VMread(uint64_t virtualAddress, word_t* value) {
    word_t next_address;
    word_t current_address = 0;
    uint64_t offset = AdressOffset(virtualAddress, 0);
    PMread(0 + offset, &next_address);
    int depth = 1;
    readRec(virtualAddress , value, depth,
            &next_address, current_address, getPageRoute(virtualAddress));
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    return 1;
}
