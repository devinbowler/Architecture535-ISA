#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

// REGISTER FUNCTIONS
REGISTERS *init_registers() {
  REGISTERS *registers = malloc(sizeof(REGISTERS));

  registers->R1 = 0;
  registers->R2 = 3;
  registers->R3 = 4;
  registers->R4 = 5;
  registers->R5 = 0;
  registers->R6 = 0;
  registers->R7 = 0;
  registers->R8 = 0;
  registers->R9 = 0;
  registers->R10 = 0;
  registers->R11 = 0;
  registers->R12 = 0;
  registers->LR = 0;
  registers->SR = 100;
  registers->PC = 0;

  return registers;
}



// DRAM FUNCTIONS

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
}

// A function to clear memory fully.
void clearMemory(DRAM *dram){
  for (int i = 0; i < DRAM_SIZE; i++){
    dram->memory[i] = 0;
  }
}

// A function to print off a block of memory.
void viewBlockMemory(DRAM *dram, uint16_t addr, uint16_t numBlocks, char values[]){
    // For simplicity, assume a block is 4 words.
    uint16_t blockSize = 4;
    // Only view one block regardless of numBlocks.
    char temp[128];
    values[0] = '\0';

    snprintf(temp, sizeof(temp), "Memory Block [ %d ]: %d %d %d %d", 
             addr,
             dram->memory[addr],
             dram->memory[addr + 1],
             dram->memory[addr + 2],
             dram->memory[addr + 3]);
    strcat(values, temp);
}


// Updated DRAM update function with cache support.
void updateDRAM(DRAM *dram, Cache *cache) {
    if (dram->state != DRAM_IDLE) {
        // Decrement delay counter and respond with "wait".
        if (dram->delayCounter > 0) {
            dram->delayCounter--;
        }
        // When delay reaches 0, finish the access.
        if (dram->delayCounter == 0) {
            if (dram->pendingAddr >= DRAM_SIZE) {
                printf("Error: address out-of-range.");
            } else {
                if (strcmp(dram->pendingCmd, "SW") == 0) {
                    // If cache is enabled, perform a write-through.
                    if (cache != NULL) {
                        write_through(cache, dram, dram->pendingAddr, dram->pendingValue);
                    } else {
                        writeToMemory(dram, dram->pendingAddr, dram->pendingValue);
                    }
                } else if (strcmp(dram->pendingCmd, "LW") == 0) {
                    int16_t readValue;
                    if (cache != NULL) {
                        readValue = read_cache(cache, dram, dram->pendingAddr);
                    } else {
                        readValue = readFromMemory(dram, dram->pendingAddr);
                    }
                } else {
                    printf("Error: unknown DRAM command.");
                }
            }
            // Reset DRAM state.
            dram->state = DRAM_IDLE;
            dram->pendingAddr = 0;
            dram->pendingValue = 0;
            strcpy(dram->pendingCmd, "");
        }
    }
}


// CACHE FUNCTIONS

/**
 * @brief Implements the write-through, no-allocate policy
 * @param cache The cache
 * @param dram The DRAM
 * @param address the address in dram to be written
 * @param data The data to be written
 * @return 0 if a miss, 1 if a hit
 */
int write_through(Cache *cache, DRAM *dram, uint16_t address, uint16_t data) {
    uint16_t index = (address / BLOCK_SIZE) % cache->num_sets;
    uint16_t tag = address / (BLOCK_SIZE * cache->num_sets);
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
  Cache *cache = malloc(sizeof(Cache));
  if (!cache) return NULL;
  
  cache->mode = mode;  // Set the mode first
  cache->num_sets = (mode == 1) ? CACHE_SIZE : CACHE_SIZE / 2;
  
  // Allocate memory for the sets array.
  cache->sets = malloc(cache->num_sets * sizeof(Set));
  if (!cache->sets) {
    free(cache);
    return NULL;
  }
  
  // Initialize each set.
  for (int i = 0; i < cache->num_sets; i++) {
    Set *set = init_set(mode);
    if (!set) {
      // Free any allocated sets here
      free(cache->sets);
      free(cache);
      return NULL;
    }
    cache->sets[i] = *set;
    free(set);  // We copy the struct so we can free the temporary pointer
  }
  return cache;
}

/**
 * @brief Initializes the associativity field and lines of a set and creates the line struct
 * @param mode The associativity of the cache
 * @return the initialized set
 */
Set *init_set(uint16_t mode) {
  Set *set = malloc(sizeof(Set));
  if (!set) return NULL;
  
  set->associativity = mode;
  // Allocate memory for the lines array based on the associativity.
  set->lines = malloc(mode * sizeof(Line));
  if (!set->lines) {
    free(set);
    return NULL;
  }
  
  // Initialize each line.
  for (int i = 0; i < set->associativity; i++) {
    Line *line = init_line();
    if (!line) {
      // Free allocated lines here (omitted for brevity)
      free(set->lines);
      free(set);
      return NULL;
    }
    set->lines[i] = *line;
    free(line);
  }
  return set;
}

/**
 * @brief Initializes the fields and data of a line and creates the line struct
 * @return the initialized line 
 */
Line *init_line() {
  Line *line = malloc(sizeof(Line));
  if (!line) return NULL;
  line->valid = 0;
  line->tag = 0;
  line->lru = 0;
  memset(line->data, 0, sizeof(line->data));
  return line;
}

/**
 * @brief Reads cache, sets valid bit, and evicts the old/least recently used element
 * @param cache the cache
 * @param dram the DRAM
 * @param address the address to be read
 * @return The data at the calculated offset
 */
uint16_t read_cache(Cache *cache, DRAM *dram, uint16_t address) {
    uint16_t index = (address / BLOCK_SIZE) % cache->num_sets;
    uint16_t tag = address / (BLOCK_SIZE * cache->num_sets);
    uint16_t offset = address & (BLOCK_SIZE - 1);
    Set *set = &cache->sets[index];
    for (int i = 0; i < cache->mode; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            if (cache->mode == 2) {
                set->lines[i].lru = 0;
                set->lines[1 - i].lru = 1;
            }
            return set->lines[i].data[offset];
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
    return line_to_replace->data[offset];
}

/**
 * @brief Clears the cache by setting all of the line data to 0
 * @param cache The cache to be cleared
 */
void clear_cache(Cache *cache) {
  for(uint16_t i = 0; i < cache->num_sets; i++) {
    for(uint16_t j = 0; j < cache->mode; j++) {
      memset(cache->sets[i].lines[j].data, 0, sizeof(cache->sets[i].lines[j].data));
      cache->sets[i].lines[j].valid = 0;
      cache->sets[i].lines[j].tag = 0;
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
