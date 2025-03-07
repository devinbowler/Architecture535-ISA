#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Define some variables that will be used for memeory.
#define DRAM_SIZE 60000
#define DRAM_DELAY 100


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
 * @return the evicted element if successful, -1 if unsuccessful
 */
int LRU(int* cache, int element) {
  return 0;
}

/**
 * @brief Implements the write-through, no-allocate policy
 *
 * @param element An element to be written
 *
 * @return 1 if successful, 0 if unsuccessful
 */
int write_through(int element) {
  return -1;
}

/**
 * @brief Initializes the cache with an array of -1s of size 64 (4 words)
 * @param mode The mapping mode - 0 is Direct-Mapping and 1 is Two-Way Set Associative
 * @return the initialized cache
 */
int* init_cache(int mode) {

}



int main(void){
  printf("DRAM_SIZE: %d\n", DRAM_SIZE);

  return 0;
}
