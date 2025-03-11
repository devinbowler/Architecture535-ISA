#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

// Define some variables that will be used for memeory.
#define DRAM_SIZE 50000
#define DRAM_DELAY 10
<<<<<<< HEAD
#define MAX_MEM_VIEW 16
#define MAX_VALUE_LENGTH 128
=======
#define CACHE_SIZE 64
#define BLOCK_SIZE 4

typedef struct Cache Cache;
typedef struct Set Set;
typedef struct Line Line;
>>>>>>> origin/cache

// Define our types.
typedef struct {
  uint16_t memory[DRAM_SIZE];
} DRAM;

<<<<<<< HEAD
typedef struct {
  uint16_t clock;
  uint16_t delay;
} STATUS;


void writeToMemory(DRAM *dram, uint16_t addr, int16_t data);
uint16_t readFromMemory(DRAM *dram, uint16_t addr);
void clearMemory(DRAM *dram);
void viewBlockMemory(DRAM *dram, uint16_t addr, char values[]);


#endif
=======
// mode of 1 = Direct-Mapped, 2 = Two-Way Set Associative
struct Cache {
  struct Set *sets;
  uint16_t num_sets;
  uint16_t mode;
};

struct Set {
  struct Line *lines;
  uint16_t associativity;
};

struct Line {
  uint16_t lru; //current # in least-recently used scheme - 0 indicates most-recently used. Does not apply for direct-mapped
  uint16_t tag;
  uint16_t valid;
  uint16_t data[BLOCK_SIZE];
};

void writeToMemory(DRAM *dram, int addr, uint16_t data);
uint16_t readFromMemory(DRAM *dram, int addr);
void clearMemory(DRAM *dram);
void viewRawMemory(DRAM *dram, int addr, char *outputStr);
uint16_t LRU(Cache *cache, uint16_t element);
uint16_t write_through(uint16_t element);
Cache *init_cache(uint16_t mode);
Set *init_set(uint16_t mode);
Line *init_line();
void clear_cache(Cache *cache);
void destroy_cache(Cache *cache);
Line *read_line(Set *set, uint16_t address);
#endif
>>>>>>> origin/cache
