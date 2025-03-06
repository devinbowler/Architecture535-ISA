#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

// Define some variables that will be used for memeory.
#define DRAM_SIZE 50000
#define DRAM_DELAY 10

// Define our types.
typedef struct {
  uint16_t memory[DRAM_SIZE];
} DRAM;

void writeToMemory(DRAM *dram, int addr, uint16_t data);
uint16_t readFromMemory(DRAM *dram, int addr);
void clearMemory(DRAM *dram);
void viewRawMemory(DRAM *dram, int addr, char *outputStr);

#endif
