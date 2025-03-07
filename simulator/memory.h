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

typedef struct {
  uint16_t clock = 0;
  uint16_t delay = 0;
  int16_t
} STATUS;


void writeToMemory(DRAM *dram, uint16_t addr, int16_t data);
uint16_t readFromMemory(DRAM *dram, uint16_t addr);
void clearMemory(DRAM *dram);
void viewRawMemory(DRAM *dram, uint16_t addr, char *outputStr);

#endif
