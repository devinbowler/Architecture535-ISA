#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Define some variables that will be used for memeory.
#define DRAM_SIZE 60000
#define DRAM_DELAY 10

// Define our types.
typedef struct {
  uint16_t memory[DRAM_SIZE];
} DRAM;

// A function to write into memory at a immediate address.
void writeToMemory(DRAM *dram, int addr, uint16_t data) {
  int delay = DRAM_DELAY;
  while (delay){
    delay -= 1;
    printf("\rWrite Delay: %d\n", delay);
  }
  if (addr >= 0 && addr < DRAM_SIZE) {
    dram->memory[addr] = (uint16_t) data;
  }
}

// A function to read from a immediate address in memeory.
uint16_t readFromMemory(DRAM *dram, int addr) {
  int delay = DRAM_DELAY;
  while (delay){
    delay -= 1;
    printf("\rRead Delay: %d\n", delay);
  }
  int returnValue;
  if (addr >= 0 && addr < DRAM_SIZE) {
      returnValue = dram->memory[addr];
  }
  return returnValue;
}

// A function to clear memory fully. (This will come in once memory is dynamic.)


// A function to print off memory, but only the lines that have addresses. (Once dynamic.)


// Testing function, this is just for testing, later this will come from the tests file.
int main(void){
  DRAM dram;
  writeToMemory(&dram, 50, 870);
  uint16_t result = readFromMemory(&dram, 50);
  printf("Value at memory address: %d\n", result);
  printf("Size of DRAM: %d\n", sizeof(dram));
  return 0;
}
