#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "memory.h"

// A function to write into memory at a immediate address.
void writeToMemory(DRAM *dram, int addr, uint16_t data) {
  if (addr >= 0 && addr < DRAM_SIZE) {
    dram->memory[addr] = (uint16_t) data;
  }
}

// A function to read from a immediate address in memeory.
uint16_t readFromMemory(DRAM *dram, int addr) {
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

// A function to print off memory, but only the lines that have addresses.
void viewRawMemory(DRAM *dram, int addr, char *outputStr){
  uint16_t value = readFromMemory(dram, addr);
  for (int i = 15; i >= 0; i--) {
    outputStr[15 - i] = ((value >> i) & 1) ? '1' : '0';
  }
  outputStr[16] = '\0';
}
