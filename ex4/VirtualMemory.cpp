//TODO: (1) delete iostream
//TODO: (2) delete the ram_insert function from the header file and from the .cpp
#include <iostream>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define READ 0
#define WRITE 1


void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    clearTable(0);
    // Random Access Memory
    // -- FRAME 0 --
//    ram_insert(0, 0, 1);
//    ram_insert(0, 1, 0);
//    // -- FRAME 1 --
//    ram_insert(1, 0, 5);
//    ram_insert(1, 1, 2);
//    // -- FRAME 2 --
//    ram_insert(2, 0, 0);
//    ram_insert(2, 1, 3);
//    // -- FRAME 3 --
//    ram_insert(3, 0, 4);
//    ram_insert(3, 1, 0);
//    // -- FRAME 4 --
//    ram_insert(4, 0, 1336);
//    ram_insert(4, 1, 1337);
//    // -- FRAME 5 --
//    ram_insert(5 , 0, 0);
//    ram_insert(5, 1, 6);
//    // -- FRAME 6 --
//    ram_insert(6, 0, 0);
//    ram_insert(6, 1, 6);
//    // -- FRAME 7 --
//    ram_insert(7, 0, 8887);
//    ram_insert(7, 1, 8888);


}

u_int64_t abs(u_int64_t page_swapped_in, u_int64_t current_frame_index){
    if ((page_swapped_in - current_frame_index) < 0){
        return  current_frame_index - page_swapped_in;
    }
    return page_swapped_in - current_frame_index;
}


u_int64_t min(u_int64_t a, u_int64_t b)
{
    if (a>b){
        return b;
    }
    return a;
}


int DfsFindBlank(u_int64_t cant_be_used, int* max_frame,
                 int depth, uint64_t*  availabe_frame, u_int64_t current_frame_index,
                 u_int64_t father, u_int64_t offset, u_int64_t page_swapped_in, int* max_value,
                 u_int64_t* frame_to_evict, u_int64_t trace,u_int64_t* page_to_evict_add_in_father,
                 u_int64_t* page_to_evict){
    if (current_frame_index > *max_frame) {
        *max_frame = current_frame_index;
    }
    if (depth == TABLES_DEPTH){
        u_int64_t a =  abs(page_swapped_in, trace);
        int  page_score = min(NUM_PAGES - a , a);
        if (page_score > *max_value){
            *max_value = page_score;
            *frame_to_evict = current_frame_index;
            *page_to_evict_add_in_father = (father * PAGE_SIZE) + offset;
            *page_to_evict = trace;
        }
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
            father = current_frame_index;
            offset = i;
            trace = (trace << OFFSET_WIDTH) + offset;
            if (DfsFindBlank(cant_be_used , max_frame , depth + 1 , availabe_frame ,
                             next_frame_index, father, offset, page_swapped_in, max_value,
                             frame_to_evict, trace, page_to_evict_add_in_father, page_to_evict))
            {
                return 1;
            }
        }
    }
    if (is_all_zero && current_frame_index != cant_be_used)
    {
        *availabe_frame = current_frame_index;
        PMwrite(father * PAGE_SIZE + offset, 0);
        return 1;
    }
    else
    {
        return 0;
    }
}



uint64_t fetchBlock(u_int64_t cant_be_used, u_int64_t page_swapped_in){
    int max_value = -1;
    int max_frame = 0, depth = 0;
    u_int64_t availabe_frame = 0;
    u_int64_t current_frame = 0;
    u_int64_t father = 0;
    u_int64_t offset = 0;
    u_int64_t frame_to_evict;
    u_int64_t trace = 0;
    u_int64_t page_to_evict_add_in_father;
    u_int64_t page_to_evict = 0;
    // OPTIONS 1 AND 2
    if (DfsFindBlank(cant_be_used, &max_frame, depth, &availabe_frame, current_frame, father,
                     offset, page_swapped_in, &max_value, &frame_to_evict, trace,
                     &page_to_evict_add_in_father, &page_to_evict)){
        return availabe_frame;
    }
    if (max_frame < NUM_FRAMES - 1)
    {
        return max_frame + 1;
    }
    // OPTION 3
    PMwrite(page_to_evict_add_in_father, 0);
    PMevict(frame_to_evict, page_to_evict);
    return frame_to_evict;
}



uint64_t AdressOffset(uint64_t virtualAddress, int depth){
    depth++;
    return virtualAddress >> (VIRTUAL_ADDRESS_WIDTH -  (depth * OFFSET_WIDTH));
}

uint64_t AddressSlicer(uint64_t addr, int depth)
{
    depth++;
    //return (addr % (1 << ((TABLES_DEPTH - depth) * OFFSET_WIDTH)));
    return addr % (1 << (VIRTUAL_ADDRESS_WIDTH -  (depth * OFFSET_WIDTH)));
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
 * @param current_frame_index  The pointer to the current frame.
 */
void readRec(uint64_t virtualAddress, word_t* value, int depth,
        word_t* next_address, word_t current_frame_index, uint64_t restoredFrameIndex,
                                                                uint64_t page_swapped_in, int rw)
{
    if (*next_address == 0)
    {
        if (TABLES_DEPTH == depth)
        {
            PMrestore(current_frame_index, restoredFrameIndex);
            uint64_t offset = AdressOffset(virtualAddress, depth);
            if (rw == READ)
            {
                PMread(current_frame_index * PAGE_SIZE + offset, next_address);
                *value = *next_address;
            }
            else // WRITE
            {
                PMwrite(current_frame_index * PAGE_SIZE + offset, *value);
            }
            return;
        }
        uint64_t unused_frame = fetchBlock(current_frame_index, page_swapped_in);
        clearTable(unused_frame);
        uint64_t offset = AdressOffset(virtualAddress, depth);
        PMwrite((current_frame_index) * PAGE_SIZE + offset, unused_frame);
        *next_address = unused_frame;
        readRec(virtualAddress, value, depth,
                next_address, current_frame_index, restoredFrameIndex, page_swapped_in, rw);
    }
    else if (TABLES_DEPTH == depth)
    {
        *value = *next_address;
        return;
    }
    else if (*next_address != 0)
    {
        virtualAddress = AddressSlicer(virtualAddress, depth); // slicing the next_address
        depth++;
        uint64_t offset = AdressOffset(virtualAddress, depth);
        current_frame_index = *next_address;
        if (TABLES_DEPTH == depth && rw == WRITE)
        {
            PMwrite((*next_address) * PAGE_SIZE + offset, *value);
            return;
        }
        PMread((*next_address) * PAGE_SIZE + offset, next_address);
        readRec(virtualAddress, value, depth,
                next_address, current_frame_index, restoredFrameIndex, page_swapped_in, rw);
    }
}

uint64_t getLogicPageFromLogigAdress(uint64_t virtualAddress){
    return  virtualAddress >> OFFSET_WIDTH;
}

int VMread(uint64_t virtualAddress, word_t* value)
{
    int depth = 0;
    uint64_t page_swapped_in = getLogicPageFromLogigAdress(virtualAddress);
    word_t next_address;
    word_t current_address = 0;
    uint64_t offset = AdressOffset(virtualAddress, depth);
    PMread(0 + offset, &next_address);
    readRec(virtualAddress , value, depth,
            &next_address, current_address, getPageRoute(virtualAddress), page_swapped_in, READ);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value)
{
    int depth = 0;
    uint64_t page_swapped_in = getLogicPageFromLogigAdress(virtualAddress);
    word_t next_address;
    word_t current_address = 0;
    uint64_t offset = AdressOffset(virtualAddress, depth);
    PMread(0 + offset, &next_address);
    readRec(virtualAddress , &value, depth,
            &next_address, current_address, getPageRoute(virtualAddress), page_swapped_in, WRITE);
    return 1;
}
