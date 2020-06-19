#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define READ 0
#define WRITE 1
#define FAILURE 0
#define SUCCESS 1

void clearTable(uint64_t frameIndex)
{
    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
    {
        PMwrite(frameIndex * PAGE_SIZE + i , 0);
    }
}

void VMinitialize()
{
    clearTable(0);
}

uint64_t abs(uint64_t page_swapped_in , uint64_t current_frame_index)
{
    if (((long)page_swapped_in - (long)current_frame_index) < 0)
    {
        return current_frame_index - page_swapped_in;
    }
    return page_swapped_in - current_frame_index;
}


uint64_t min(uint64_t a , uint64_t b)
{
    if (a > b)
    {
        return b;
    }
    return a;
}


int DfsFindBlank(uint64_t cant_be_used , int *max_frame ,
                 int depth , uint64_t *availabe_frame , uint64_t current_frame_index ,
                 uint64_t father , uint64_t offset , uint64_t page_swapped_in , int *max_value ,
                 uint64_t *frame_to_evict , uint64_t trace ,
                 uint64_t *page_to_evict_add_in_father ,
                 uint64_t *page_to_evict)
{
    if ((long)current_frame_index > (long)*max_frame)
    {
        *max_frame = current_frame_index;
    }
    if (depth == TABLES_DEPTH)
    {
        uint64_t a = abs(page_swapped_in , trace);
        int page_score = min(NUM_PAGES - a , a);
        if (page_score > *max_value)
        {
            *max_value = page_score;
            *frame_to_evict = current_frame_index;
            *page_to_evict_add_in_father = (father * PAGE_SIZE) + offset;
            *page_to_evict = trace;
        }
        return 0;
    }
    bool is_all_zero = true;
    trace = (trace << OFFSET_WIDTH);
    for (uint64_t i = 0; i < PAGE_SIZE; ++i)
    {
        word_t next_frame_index;
        PMread(current_frame_index * PAGE_SIZE + i , &next_frame_index);
        if (next_frame_index != 0)
        {
            is_all_zero = false;
            father = current_frame_index;
            offset = i;
            uint64_t trace1 = trace + offset;
            if (DfsFindBlank(cant_be_used , max_frame , depth + 1 , availabe_frame ,
                             next_frame_index , father , offset , page_swapped_in , max_value ,
                             frame_to_evict , trace1 , page_to_evict_add_in_father , page_to_evict))
            {
                return 1;
            }
        }
    }
    if (is_all_zero && current_frame_index != cant_be_used)
    {
        *availabe_frame = current_frame_index;
        PMwrite(father * PAGE_SIZE + offset , 0);
        return 1;
    }
    else
    {
        return 0;
    }
}


uint64_t fetchBlock(uint64_t cant_be_used , uint64_t page_swapped_in)
{
    int max_value = -1;
    int max_frame = 0 , depth = 0;
    uint64_t availabe_frame = 0;
    uint64_t current_frame = 0;
    uint64_t father = 0;
    uint64_t offset = 0;
    uint64_t frame_to_evict;
    uint64_t trace = 0;
    uint64_t page_to_evict_add_in_father;
    uint64_t page_to_evict = 0;
    // OPTIONS 1 AND 2
    if (DfsFindBlank(cant_be_used , &max_frame , depth , &availabe_frame , current_frame , father ,
                     offset , page_swapped_in , &max_value , &frame_to_evict , trace ,
                     &page_to_evict_add_in_father , &page_to_evict))
    {
        return availabe_frame;
    }
    if (max_frame < NUM_FRAMES - 1)
    {
        return max_frame + 1;
    }
    // OPTION 3
    PMwrite(page_to_evict_add_in_father , 0);
    PMevict(frame_to_evict , page_to_evict);
    return frame_to_evict;
}


uint64_t AdressOffset(uint64_t virtualAddress , int depth)
{
    return virtualAddress >>( (TABLES_DEPTH - depth) *  OFFSET_WIDTH);
}

uint64_t AddressSlicer(uint64_t addr , int depth)
{
    return addr % (1 << (TABLES_DEPTH - depth) *  OFFSET_WIDTH);
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
void readRec(uint64_t virtualAddress , word_t *value , int depth ,
             word_t *next_address , word_t current_frame_index , uint64_t restoredPageIndex ,
             uint64_t page_swapped_in , int rw, bool IS_CLEAN)
{
    //we arrived to leafe (frame which store the page data)
    if (TABLES_DEPTH == depth)
    {
        // if the block was cleand and need to restored from HD
        if (IS_CLEAN) {
            PMrestore(current_frame_index , restoredPageIndex);
        }
        uint64_t offset = AdressOffset(virtualAddress , depth);
        //READ
        if (rw == READ)
        {
            PMread(current_frame_index * PAGE_SIZE + offset , next_address);
            *value = *next_address;

        }
        else // WRITE
        {
            PMwrite(current_frame_index * PAGE_SIZE + offset , *value);
        }
        return;
    }
    // if need to locate new frame
    if (*next_address == 0)
    {
        // find the frame to erase
        uint64_t unused_frame = fetchBlock(current_frame_index , page_swapped_in);
        //erase block
        clearTable(unused_frame);
        // if we erase the frame that should hold the page vars (the leafe) we should restore date from HD later
        if (depth == TABLES_DEPTH - 1) {IS_CLEAN = true ;}
        // slice offset
        uint64_t offset = AdressOffset(virtualAddress , depth);
        //write to ram address the number of frame (the adress of the son)
        PMwrite((current_frame_index) * PAGE_SIZE + offset , unused_frame);
        *next_address = unused_frame;
        readRec(virtualAddress , value , depth ,
                next_address , current_frame_index , restoredPageIndex , page_swapped_in , rw, IS_CLEAN);
    }
    // if need to jump to the next table
    else if (*next_address != 0)
    {
        virtualAddress = AddressSlicer(virtualAddress , depth); // slicing the next_address
        depth++;
        uint64_t offset = AdressOffset(virtualAddress , depth);
        current_frame_index = *next_address;
        PMread((*next_address) * PAGE_SIZE + offset , next_address);
        readRec(virtualAddress , value , depth ,
                next_address , current_frame_index , restoredPageIndex , page_swapped_in , rw, IS_CLEAN);
    }
}


uint64_t getLogicPageFromLogigAdress(uint64_t virtualAddress)
{
    return virtualAddress >> OFFSET_WIDTH;
}


int checkAddressValidy(uint64_t addr)
{

    return !((long)addr >= 0 && (long)addr < VIRTUAL_MEMORY_SIZE);
}


int VMread(uint64_t virtualAddress , word_t *value)
{
    if (checkAddressValidy(virtualAddress))
    {
        return FAILURE; // failed
    }
    int depth = 0;
    uint64_t page_swapped_in = getLogicPageFromLogigAdress(virtualAddress);
    word_t next_address;
    word_t current_address = 0;
    uint64_t offset = AdressOffset(virtualAddress , depth);
    PMread(0 + offset , &next_address);
    bool  is_clean = false;
    readRec(virtualAddress , value , depth ,
            &next_address , current_address , getPageRoute(virtualAddress) , page_swapped_in ,
            READ, is_clean);
    return SUCCESS;
}


int VMwrite(uint64_t virtualAddress , word_t value)
{

    if (checkAddressValidy(virtualAddress))
    {
        return FAILURE; // failed
    }

    int depth = 0;
    uint64_t page_swapped_in = getLogicPageFromLogigAdress(virtualAddress);
    word_t next_address;
    word_t current_address = 0;
    uint64_t offset = AdressOffset(virtualAddress , depth);
    PMread(0 + offset , &next_address);
    bool  is_clean = false;
    readRec(virtualAddress , &value , depth ,
            &next_address , current_address , getPageRoute(virtualAddress) , page_swapped_in ,
            WRITE, is_clean);
    return SUCCESS;
}