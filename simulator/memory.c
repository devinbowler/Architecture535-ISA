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
void viewBlockMemory(DRAM *dram, uint16_t addr, char values[]){
  // Loop to go through 16 values and add them to an array.
  uint16_t start = (addr >= 7) ? addr - 7 : 0;
  char *ptr = values;
  for (int i = 0; i < 16; i++){
    ptr += sprintf(ptr, "%04x ", dram->memory[start + i]);
  }
}
