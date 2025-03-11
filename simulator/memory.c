#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

/**
 * @brief Implements the Least Recently Used eviction policy for cache
 * @param cache The cache
 * @param element The element that is being added
 * @return the evicted element
 */
uint16_t LRU(Cache *cache, uint16_t element) {
  uint16_t index = cache->least_recently;
  cache->least_recently = index+1 < CACHE_SIZE ? index+1 : 0;
  cache->memory[index] = element;
}

/**
 * @brief Implements the write-through, no-allocate policy
 * @param element An element to be written
 * @return 1 if successful, 0 if unsuccessful
 */
uint16_t write_through(uint16_t element) {
  if()
}

/**
 * @brief Initializes the cache with an array of 0s of size 64 (4 words)
 * @param mode The mapping mode - 1 is Direct-Mapping and 2 is Two-Way Set Associative
 * @return the initialized cache if successful, NULL otherwise
 */
Cache *init_cache(uint16_t mode) {
  if(mode != 1 && mode != 2) {
    printf("Invalid mode.");
    return NULL;
  }
  Cache *cache = malloc(sizeof(Cache));
  if(!cache) return NULL;
  cache->num_sets = (cache->mode == 1) ? CACHE_SIZE : CACHE_SIZE / 2
  for(int i = 0; i < cache->num_sets; i++) {
    cache->sets[i] = init_set(mode);
  }
  clear_cache(cache);
  cache->mode = mode;
  return cache;
}

/**
 * @brief Initializes the associativity field and lines of a set and creates the line struct
 * @param mode The associativity of the cache
 * @return the initialized set
 */
Set *init_set(uint16_t mode) {
  Set *set = malloc(sizeof(Set));
  set->associativity = mode;
  if(mode == 2) {
    set->lines[0] = init_line();
    set->lines[1] = init_line();
  } else {
    set->lines[0] = init_line();
  }
  return set;
}

/**
 * @brief Initializes the fields and data of a line and creates the line struct
 * @return the initialized line 
 */
Line *init_line() {
  Line *line = malloc(sizeof(Line));
  line->valid = 0;
  line->tag = 0;
  line->data = {0,0,0,0};
  return line;
}

/**
 * @brief Reads the line in cache, changes lru and valid bit accordingly if applicable, and returns the line
 * @param set the set in cache
 * @param address the address to be read
 * @return the line in cache if successful, NULL otherwise
 */
Line *read_line(Cache *cache, uint16_t address) {
  if(!set) return NULL;
  Line *line = cache->sets[address]
  if(!line) return NULL;
  if(line->valid == 0) {
    line->valid = 1;
    line->data = readFromMemory(dram, address);
  }
  if(set->associativity == 2) {
    line->lru = 0;
    set->lines[1]->lru = 1;
  }
  return line;
}

/**
 * @brief Clears the cache by setting all of the line data to 0
 * @param cache The cache to be cleared
 */
void clear_cache(Cache *cache) {
  for(uint16_t i = 0; i < cache->num_sets; i++) {
    cache->sets[i]->lines[0]->data = {0,0,0,0};
    cache->sets[i]->lines[0]->valid = 0;
    cache->sets[i]->lines[0]->tag = 0;
    cache->sets[i]->lines[1]->data = {0,0,0,0};
    cache->sets[i]->lines[1]->valid = 0;
    cache->sets[i]->lines[1]->tag = 0;
  }
}

/**
 * @brief Frees all of the associated with a cache, including the sets array, lines array, and the cache struct itself
 * @param cache The cache to be destroyed
 */
void destroy_cache(Cache *cache) {
  if(!cache) return;
  for (uint16_t i = 0; i < cache->num_sets; i++) {
    free(cache->sets[i].lines);
  }
  free(cache->sets);
  free(cache);
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
