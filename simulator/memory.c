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
 * @param cache The cache
 * @param dram The dram
 * @param address the address in dram to be written
 * @param data The data to be written
 * @return 0 if a miss, 1 if a hit
 */
int write_through(Cache *cache, DRAM *dram, uint16_t address, uint16_t data) {
    uint16_t index = (address / BLOCK_SIZE) % num_sets;
    uint16_t tag = address / (BLOCK_SIZE * num_sets);
    uint16_t offset = address & (BLOCK_SIZE - 1);
    Set *set = &cache->sets[index];
    for (int i = 0; i < cache->mode; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            set->lines[i].data[offset] = data;
            writeToMemory(dram, address, data);
            return 1;
        }
    }
    writeToMemory(dram, address, data);
    return 0;
}

/**
 * @brief Initializes the cache with an array of 0s of size 64 (4 words)
 * @param mode The mapping mode - 1 is Direct-Mapping and 2 is Two-Way Set Associative
 * @return The initialized cache if successful, NULL otherwise
 */
Cache *init_cache(uint16_t mode) {
  if(mode != 1 && mode != 2) {
    printf("Invalid mode.");
    return NULL;
  }
  Cache *cache = (Cache*)malloc(sizeof(Cache));
  if(!cache) return NULL;
  cache->num_sets = (cache->mode == 1) ? CACHE_SIZE : CACHE_SIZE / 2
  for(int i = 0; i < cache->num_sets; i++) {
    cache->sets[i] = *init_set(mode);
  }
  cache->mode = mode;
  return cache;
}

/**
 * @brief Initializes the associativity field and lines of a set and creates the line struct
 * @param mode The associativity of the cache
 * @return the initialized set
 */
Set *init_set(uint16_t mode) {
  Set *set = (Set*)malloc(sizeof(Set));
  set->associativity = mode;
  for(int i = 0; i < set->associativity; i++) {
    set->lines[i] = *init_line();
  }
  return set;
}

/**
 * @brief Initializes the fields and data of a line and creates the line struct
 * @return the initialized line 
 */
Line *init_line() {
  Line *line = (Line*)malloc(sizeof(Line));
  line->valid = 0;
  line->tag = 0;
  memset(line->data, 0, sizeof(line->data));
  return line;
}

/**
 * @brief Reads the line in cache, changes lru and valid bit accordingly if applicable, and returns the line
 * @param cache the cache
 * @param dram the DRAM
 * @param address the address to be read
 * @return the line in cache if successful, NULL otherwise
 */
Line *read_line(Cache *cache, DRAM *dram, uint16_t address) {
    uint16_t index = (address / BLOCK_SIZE) % num_sets;
    uint16_t tag = address / (BLOCK_SIZE * num_sets);
    uint16_t offset = address & (BLOCK_SIZE - 1);
    Set *set = &cache->sets[index];
    for (int i = 0; i < cache->mode; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            if (cache->mode == 2) {
                set->lines[i].lru = 0;
                set->lines[1 - i].lru = 1;
            }
            return &set->lines[i];
        }
    }
    Line *line_to_replace = &set->lines[0];
    if (cache->mode == 2) {
        line_to_replace = (set->lines[0].lru == 1) ? &set->lines[0] : &set->lines[1];
    }
    line_to_replace->tag = tag;
    line_to_replace->valid = 1;
    uint16_t block_start = address - offset;
    for (int i = 0; i < BLOCK_SIZE; i++) {
        line_to_replace->data[i] = readFromMemory(dram, block_start + i);
    }

    return line_to_replace;
}

/**
 * @brief Clears the cache by setting all of the line data to 0
 * @param cache The cache to be cleared
 */
void clear_cache(Cache *cache) {
  for(uint16_t i = 0; i < cache->num_sets; i++) {
    for(uint16_t j = 0; j < cache->mode; j++) {
      memset(cache->sets[i].lines[j].data, 0, sizeof(cache->sets[i].lines[j].data));
      cache->sets[i]->lines[j]->valid = 0;
      cache->sets[i]->lines[j]->tag = 0;
    }
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
