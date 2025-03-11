#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "memory.h"

// A function to write into memory at a immediate address.
void writeToMemory(DRAM *dram, uint16_t addr, int16_t data) {
  if (addr >= 0 && addr < DRAM_SIZE) {
    dram->memory[addr] = (int16_t) data;
  }
}

// A function to read from a immediate address in memeory.
uint16_t readFromMemory(DRAM *dram, uint16_t addr) {
  if (addr >= 0 && addr < DRAM_SIZE) {
      return dram->memory[addr];
  }
  return 0;
}

// A function to clear memory fully.
void clearMemory(DRAM *dram){
  for (int i = 0; i < DRAM_SIZE; i++){
    dram->memory[i] = 0;
  }
}

// A function to print off a block of memory.
void viewBlockMemory(DRAM *dram, uint16_t addr, uint16_t numBlocks, char values[]){
    // For simplicity, assume a block is 4 words.
    uint16_t blockSize = 4;
    // Only view one block regardless of numBlocks.
    char temp[128];
    values[0] = '\0';

    snprintf(temp, sizeof(temp), "Memory Block [ %d ]: %d %d %d %d", 
             addr,
             dram->memory[addr],
             dram->memory[addr + 1],
             dram->memory[addr + 2],
             dram->memory[addr + 3]);
    strcat(values, temp);
}
