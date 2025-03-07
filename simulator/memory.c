#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
// A function to write into memory at a immediate address.


// A function to read from a immediate address in memeory.


// A function to clear memory fully.


// A function to print off memory, but only the lines that have addresses.


// Testing function, this is just for testing, later this will come from the tests file.

/**
 * @brief Implements the Least Recently Used eviction policy for cache
 *
 * @param cache The cache
 * @param element The element that is being added
 *
 * @return the evicted element
 */
uint16_t LRU(Cache *cache, uint16_t element) {
  uint16_t index = cache->least_recently;
  cache->least_recently = index+1 < CACHE_SIZE ? index+1 : 0;
  cache->memory[index] = element;
}

/**
 * @brief Implements the write-through, no-allocate policy
 *
 * @param element An element to be written
 *
 * @return 1 if successful, 0 if unsuccessful
 */
uint16_t write_through(uint16_t element) {
  return 0;
}

/**
 * @brief Initializes the cache with an array of 0s of size 64 (4 words)
 *
 * @param mode The mapping mode - 0 is Direct-Mapping and 1 is Two-Way Set Associative
 *
 * @return the initialized cache
 */
Cache *init_cache(uint16_t mode) {
  Cache *cache;
  for(uint16_t i = 0; i < CACHE_SIZE; i++) {
    for(uint16_t j = 0; j < CACHE_SIZE; j++) {
      cache->memory[i][j] = 0;
    }
    cache->mode = mode;
  }
  return cache
}

/**
 * @brief Clears the cache by setting all of it to 0.
 *
 * @param cache The cache to be cleared
 *
 */
void clear_cache(Cache *cache) {
  for(int i = 0; i < CACHE_SIZE; i++) {
    for(int j = 0; j < CACHE_SIZE; j++) {
      cache->memory[i][j] = 0;
    }
  }
}


int main(void){
  printf("DRAM_SIZE: %d\n", DRAM_SIZE);

  return 0;
}
