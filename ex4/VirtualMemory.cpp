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
    ram_insert(4 , 0, 0);
    ram_insert(4,1,0);

    ram_insert(7, 0, 0);
    ram_insert(7, 1, 1001);
    PMevict(7, 3);
}

int DfsFindBlank(u_int64_t cant_be_used, int* max_frame, int depth, uint64_t*  availabe_frame,
                                                                        u_int64_t current_frame_index){
    std::cout << "this is current frame "<< current_frame_index << "\n";
    if (current_frame_index > *max_frame) {
        *max_frame = current_frame_index;
    }
    if (depth == TABLES_DEPTH){
        return 0;
    }
    bool is_all_zero = true;
    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
    {
        word_t next_frame_index;
        PMread(current_frame_index * PAGE_SIZE + i , &next_frame_index);
        if (next_frame_index != 0)
        {
            is_all_zero = false;
            if (DfsFindBlank(cant_be_used , max_frame , depth + 1 , availabe_frame ,
                             next_frame_index))
            {
                return 1;
            }
        }
    }

    if (is_all_zero && current_frame_index != cant_be_used)
    {
        std::cout << "not availabe " <<  cant_be_used << std::endl;
        std::cout << "current frame  " <<  current_frame_index << std::endl;

        *availabe_frame = current_frame_index;
        return 1;
    }
    else
    {
        return 0;
    }


}

uint64_t fetchBlock(u_int64_t cant_be_used){
    int max_frame = 0, depth = 0;
    u_int64_t availabe_frame = 0;
    u_int64_t current_frame = 0;
    if (DfsFindBlank(cant_be_used, &max_frame, depth, &availabe_frame, current_frame)){
//        std::cout << "should be here because no frame found\n";
        return availabe_frame;
    }
    if (max_frame < NUM_FRAMES){
        std::cout << max_frame << "\n";
        return max_frame + 1;
    }
}

uint64_t AdressOffset(uint64_t virtualAddress, int depth){
    return virtualAddress >>  (VIRTUAL_ADDRESS_WIDTH -  (depth * OFFSET_WIDTH) );
}

uint64_t AddressSlicer(uint64_t addr, int depth)
{
    //return (addr % (1 << ((TABLES_DEPTH - depth) * OFFSET_WIDTH)));
    return addr % (1 << (VIRTUAL_ADDRESS_WIDTH -  (depth * OFFSET_WIDTH) ));
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
 * @param current_address  The pointer to the current frame.
 */
void readRec(uint64_t virtualAddress, word_t* value, int depth,
        word_t* next_address, word_t current_address, uint64_t restoredFrameIndex){
    if (*next_address == 0)
    {
        uint64_t unused_frame = fetchBlock(current_address);
        clearTable(unused_frame);
        if (TABLES_DEPTH == depth)
        {
            PMrestore(unused_frame, restoredFrameIndex);
            uint64_t offset = AdressOffset(virtualAddress, depth);
            PMread(unused_frame * PAGE_SIZE + offset, next_address);
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
        *value = *next_address;
        return;
    }
    else if (*next_address != 0)
    {
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
//    std::cout << "offset " << AdressOffset(virtualAddress , 0) << std::endl;
//    virtualAddress = AddressSlicer(virtualAddress, 1);
//    std::cout << "new adress " << virtualAddress << std::endl;

    word_t next_address;
    word_t current_address = 0;
    uint64_t offset = AdressOffset(virtualAddress, 0);
    PMread(0 + offset, &next_address);
    int depth = 1;
    readRec(virtualAddress , value, depth,
            &next_address, current_address, getPageRoute(virtualAddress));
    print_ram();
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    return 1;
}
