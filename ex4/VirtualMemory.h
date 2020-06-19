#pragma once

#include "MemoryConstants.h"

/*
 * Initialize the virtual memory
 */
void VMinitialize();

/* reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t* value);

/* writes a word to the given virtual address
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */

int VMwrite(uint64_t virtualAddress, word_t value);


void print_ram();

uint64_t AdressOffset(uint64_t virtualAddress , int depth);

uint64_t AddressSlicer(uint64_t addr , int depth);

uint64_t AddressSlicer2(uint64_t addr , int depth);

uint64_t AdressOffset2(uint64_t virtualAddress , int depth);