#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

// Define some variables that will be used for memeory.
#define DRAM_SIZE 50000
#define DRAM_DELAY 10
#define CACHE_SIZE 64

// Define our types.
typedef struct {
  uint16_t memory[DRAM_SIZE];
  uint16_t least_recently;
} DRAM;

typedef struct {
  uint16_t cache[CACHE_SIZE][CACHE_SIZE];
  uint16_t mode;
} Cache;

typedef struct {

} Set;

typedef struct {

} Line;

void writeToMemory(DRAM *dram, int addr, uint16_t data);
uint16_t readFromMemory(DRAM *dram, int addr);
void clearMemory(DRAM *dram);
void viewRawMemory(DRAM *dram, int addr, char *outputStr);
int LRU(Cache *cache, int element);
int write_through(int element);
Cache *init_cache(int mode);
int clear_cache(Cache *cache);
#endif