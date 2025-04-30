#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "globals.h"

// REGISTER FUNCTIONS
REGISTERS *init_registers() {
  REGISTERS *registers = malloc(sizeof(REGISTERS));
  registers->R[0] = 0;  // R0 is always 0
  registers->R[1] = 1;  // R1 starts with 1
  
  // Initialize general registers to 0
  for (int i = 2; i < 13; i++) {
    registers->R[i] = 0;
  }
  
  // Initialize special registers
  registers->R[13] = 0;  // LR (Link Register)
  registers->R[14] = 0;  // SR (Status Register)
  registers->R[15] = 0;  // PC (Program Counter)

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
  } else {
    printf("Error: Read address %u out of range\n", addr);
    return 0;
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

// Unified memory access functions

/**
 * @brief Reads from memory, using cache if available
 * @param cache The cache, can be NULL to bypass cache
 * @param dram The DRAM
 * @param address The address to read from 
 * @return The data at the specified address
 */
uint16_t memory_read(Cache *cache, DRAM *dram, uint16_t address) {
    if (address >= DRAM_SIZE) {
        printf("Error: read address %u out-of-range.\n", address);
        return 0;
    }
    
    if (CACHE_ENABLED && cache != NULL) {
        // Use cache for reads
        return read_cache(cache, dram, address);
    } else {
        // Direct DRAM read
        printf("[MEM_READ] Cache disabled or not available, reading directly from DRAM\n");
        return readFromMemory(dram, address);
    }
}

/**
 * @brief Writes to memory, using write-through if cache is available
 * @param cache The cache, can be NULL to bypass cache
 * @param dram The DRAM
 * @param address The address to write to
 * @param data The data to write
 */
void memory_write(Cache *cache, DRAM *dram, uint16_t address, uint16_t data) {
    if (address >= DRAM_SIZE) {
        printf("Error: write address %u out-of-range.\n", address);
        return;
    }
        
    if (CACHE_ENABLED && cache != NULL) {
        // Use write-through for cache writes
        write_through(cache, dram, address, data);
    } else {
        // Direct DRAM write
        printf("[MEM_WRITE] Cache disabled or not available, writing directly to DRAM\n");
        writeToMemory(dram, address, data);
    }
    
    // Print memory update for UI
    printf("[MEM]%d:%d\n", address, data);
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
                    // Use the unified memory_write function
                    memory_write(cache, dram, dram->pendingAddr, dram->pendingValue);
                } else if (strcmp(dram->pendingCmd, "LW") == 0) {
                    int16_t readValue;
                    // Use the unified memory_read function
                    readValue = memory_read(cache, dram, dram->pendingAddr);
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
 * Initializes a cache with the specified mode (direct-mapped or set-associative)
 */
Cache *init_cache(uint16_t mode) {
    if (mode != 1 && mode != 2) {
        printf("Error: Invalid cache mode %u (must be 1 or 2)\n", mode);
        return NULL;
    }
    
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    if (!cache) return NULL;
    
    cache->mode = mode;
    cache->num_sets = (mode == 1) ? 16 : 8;  // Fixed cache size to 16 sets for direct-mapped
    
    // Allocate memory for the sets
    cache->sets = (Set *)malloc(cache->num_sets * sizeof(Set));
    if (!cache->sets) {
        free(cache);
        return NULL;
    }
    
    // Initialize each set
    for (uint16_t i = 0; i < cache->num_sets; i++) {
        cache->sets[i].associativity = mode;
        cache->sets[i].lines = (Line *)malloc(mode * sizeof(Line));
        
        if (!cache->sets[i].lines) {
            // Clean up previously allocated memory
            for (uint16_t j = 0; j < i; j++) {
                free(cache->sets[j].lines);
            }
            free(cache->sets);
            free(cache);
            return NULL;
        }
        
        // Initialize each line in the set
        for (uint16_t j = 0; j < mode; j++) {
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].tag = 0;
            cache->sets[i].lines[j].lru = j; // For set-associative cache
            
            // Initialize data blocks to zero
            for (uint16_t k = 0; k < BLOCK_SIZE; k++) {
                cache->sets[i].lines[j].data[k] = 0;
            }
        }
    }
    
    printf("[CACHE_INIT] Created cache with %u sets, mode %u\n", 
           cache->num_sets, cache->mode);
    return cache;
}

/**
 * Reads data from the cache, or from memory if not cached (with allocation)
 */
uint16_t read_cache(Cache *cache, DRAM *dram, uint16_t address) {
    // Verify cache is valid
    if (!cache) {
        printf("[CACHE_ERROR] Null cache pointer in read_cache\n");
        return readFromMemory(dram, address);
    }
    
    // Calculate cache addressing
    uint16_t block_offset = address % BLOCK_SIZE;
    uint16_t block_address = address - block_offset; // Address aligned to block boundary
    uint16_t set_index = (block_address / BLOCK_SIZE) % cache->num_sets;
    uint16_t tag = block_address / (BLOCK_SIZE * cache->num_sets);
    
    printf("[CACHE_DEBUG] Read address %u: set=%u, tag=%u, offset=%u\n", 
           address, set_index, tag, block_offset);
    
    // Get the appropriate set
    Set *set = &cache->sets[set_index];
    
    // Check for a cache hit
    for (int i = 0; i < cache->mode; i++) {
        Line *line = &set->lines[i];
        
        if (line->valid && line->tag == tag) {
            // Cache hit
            printf("[CACHE_HIT] Address %u found in cache set %u, tag %u\n", 
                   address, set_index, tag);
            
            // Output cache state for UI
            printf("[CACHE]%u:%u:%u:%u\n", set_index, 0, 1, tag);
            
            // Output all block data for UI
            for (int j = 0; j < BLOCK_SIZE; j++) {
                printf("[CACHE_DATA]%u:%u:%u:%u\n", 
                       set_index, 0, j, line->data[j]);
            }
            
            // Update LRU for set associative cache
            if (cache->mode == 2) {
                // Remember old LRU value
                uint16_t old_lru = line->lru;
                
                // Set this line as most recently used
                line->lru = 0;
                
                // Update LRU counters for other lines in the set
                for (int j = 0; j < cache->mode; j++) {
                    if (j != i && set->lines[j].lru < old_lru) {
                        set->lines[j].lru++;
                    }
                }
            }
            
            return line->data[block_offset];
        }
    }
    
    // Cache miss - find a line to use (LRU replacement)
    printf("[CACHE_MISS] Address %u not in cache\n", address);
    
    // Determine which line to replace
    Line *victim_line = NULL;
    
    if (cache->mode == 1) {
        // Direct-mapped: only one choice
        victim_line = &set->lines[0];
    } else {
        // Set associative: find invalid line or highest LRU
        uint16_t highest_lru = 0;
        victim_line = &set->lines[0]; // Default to first line
        
        for (int i = 0; i < cache->mode; i++) {
            if (!set->lines[i].valid) {
                // Found an invalid line - use it immediately
                victim_line = &set->lines[i];
                break;
            }
            
            if (set->lines[i].lru > highest_lru) {
                highest_lru = set->lines[i].lru;
                victim_line = &set->lines[i];
            }
        }
    }
    
    // If the victim line is valid, it needs to be evicted
    if (victim_line->valid) {
        printf("[CACHE_EVICT] Replacing line with tag %u in set %u\n", 
               victim_line->tag, set_index);
    }
    
    // Fill the cache line with data from memory
    victim_line->valid = 1;
    victim_line->tag = tag;
    
    // For set associative cache, update LRU status
    if (cache->mode == 2) {
        // Set this as most recently used
        uint16_t old_lru = victim_line->lru;
        victim_line->lru = 0;
        
        // Update others' LRU
        for (int i = 0; i < cache->mode; i++) {
            Line *line = &set->lines[i];
            if (line != victim_line && line->valid && line->lru < old_lru) {
                line->lru++;
            }
        }
    }
    
    // Load data from memory into cache line
    for (int i = 0; i < BLOCK_SIZE; i++) {
        uint16_t block_addr = block_address + i;
        victim_line->data[i] = readFromMemory(dram, block_addr);
    }
    
    // Output cache state for UI visualization
    printf("[CACHE]%u:%u:%u:%u\n", set_index, 0, 1, tag);
    
    // Output all block data for UI
    for (int i = 0; i < BLOCK_SIZE; i++) {
        printf("[CACHE_DATA]%u:%u:%u:%u\n", 
               set_index, 0, i, victim_line->data[i]);
    }
    
    // Return the requested data
    return victim_line->data[block_offset];
}

/**
 * Write-through cache implementation
 * Returns 1 on cache hit, 0 on cache miss
 */
int write_through(Cache *cache, DRAM *dram, uint16_t address, uint16_t data) {
    // Verify cache is valid
    if (!cache) {
        printf("[CACHE_ERROR] Null cache pointer in write_through\n");
        writeToMemory(dram, address, data);
        return 0;
    }
    
    // Calculate cache addressing
    uint16_t block_offset = address % BLOCK_SIZE;
    uint16_t block_address = address - block_offset; // Address aligned to block boundary
    uint16_t set_index = (block_address / BLOCK_SIZE) % cache->num_sets;
    uint16_t tag = block_address / (BLOCK_SIZE * cache->num_sets);
    
    printf("[CACHE_DEBUG] Write address %u: set=%u, tag=%u, offset=%u, data=%u\n", 
           address, set_index, tag, block_offset, data);
    
    // Write-through policy: always update memory
    writeToMemory(dram, address, data);
    
    // Get the appropriate set
    Set *set = &cache->sets[set_index];
    
    // Check for a cache hit
    for (int i = 0; i < cache->mode; i++) {
        Line *line = &set->lines[i];
        
        if (line->valid && line->tag == tag) {
            // Cache hit - update the cached data
            printf("[CACHE_WRITE_HIT] Updating address %u in cache\n", address);
            
            // Update the data in the cache line
            line->data[block_offset] = data;
            
            // Output cache state for UI
            printf("[CACHE]%u:%u:%u:%u\n", set_index, 0, 1, tag);
            
            // Output all block data for UI
            for (int j = 0; j < BLOCK_SIZE; j++) {
                printf("[CACHE_DATA]%u:%u:%u:%u\n", 
                       set_index, 0, j, line->data[j]);
            }
            
            // Update LRU for set associative cache
            if (cache->mode == 2) {
                // Remember old LRU value
                uint16_t old_lru = line->lru;
                
                // Set this line as most recently used
                line->lru = 0;
                
                // Update LRU counters for other lines in the set
                for (int j = 0; j < cache->mode; j++) {
                    if (j != i && set->lines[j].lru < old_lru) {
                        set->lines[j].lru++;
                    }
                }
            }
            
            return 1; // Cache hit
        }
    }
    
    // Cache miss with write-through policy
    // No write-allocate: we only write to memory
    printf("[CACHE_WRITE_MISS] Address %u not in cache (write-through, no allocate)\n", 
           address);
    
    return 0; // Cache miss
}

/**
 * Specifically for instruction fetches to use the cache
 * This is a variant of read_cache designed to work with the fetch stage
 */
uint16_t fetch_with_cache(Cache *cache, DRAM *dram, uint16_t address, bool *is_hit) {
    // Default to cache miss
    *is_hit = false;
    
    // Verify cache is valid
    if (!cache || !CACHE_ENABLED) {
        return readFromMemory(dram, address);
    }
    
    // Calculate cache addressing
    uint16_t block_offset = address % BLOCK_SIZE;
    uint16_t block_address = address - block_offset; // Address aligned to block boundary
    uint16_t set_index = (block_address / BLOCK_SIZE) % cache->num_sets;
    uint16_t tag = block_address / (BLOCK_SIZE * cache->num_sets);
    
    printf("[FETCH_CACHE] Check address %u: set=%u, tag=%u, offset=%u\n", 
           address, set_index, tag, block_offset);
    
    // Get the appropriate set
    Set *set = &cache->sets[set_index];
    
    // Check for a cache hit
    for (int i = 0; i < cache->mode; i++) {
        Line *line = &set->lines[i];
        
        if (line->valid && line->tag == tag) {
            // Cache hit for instruction fetch
            printf("[FETCH_CACHE_HIT] Address %u found in cache\n", address);
            *is_hit = true;
            
            // Update LRU for set associative cache
            if (cache->mode == 2) {
                // Remember old LRU value
                uint16_t old_lru = line->lru;
                
                // Set this line as most recently used
                line->lru = 0;
                
                // Update LRU counters for other lines in the set
                for (int j = 0; j < cache->mode; j++) {
                    if (j != i && set->lines[j].lru < old_lru) {
                        set->lines[j].lru++;
                    }
                }
            }
            
            return line->data[block_offset];
        }
    }
    
    // Cache miss - find a line to use (LRU replacement)
    printf("[FETCH_CACHE_MISS] Address %u not in cache\n", address);
    
    // Determine which line to replace
    Line *victim_line = NULL;
    
    if (cache->mode == 1) {
        // Direct-mapped: only one choice
        victim_line = &set->lines[0];
    } else {
        // Set associative: find invalid line or highest LRU
        uint16_t highest_lru = 0;
        victim_line = &set->lines[0]; // Default to first line
        
        for (int i = 0; i < cache->mode; i++) {
            if (!set->lines[i].valid) {
                // Found an invalid line - use it immediately
                victim_line = &set->lines[i];
                break;
            }
            
            if (set->lines[i].lru > highest_lru) {
                highest_lru = set->lines[i].lru;
                victim_line = &set->lines[i];
            }
        }
    }
    
    // If the victim line is valid, it needs to be evicted
    if (victim_line->valid) {
        printf("[FETCH_CACHE_EVICT] Replacing line with tag %u in set %u\n", 
               victim_line->tag, set_index);
    }
    
    // Fill the cache line with data from memory
    victim_line->valid = 1;
    victim_line->tag = tag;
    
    // For set associative cache, update LRU status
    if (cache->mode == 2) {
        // Set this as most recently used
        uint16_t old_lru = victim_line->lru;
        victim_line->lru = 0;
        
        // Update others' LRU
        for (int i = 0; i < cache->mode; i++) {
            Line *line = &set->lines[i];
            if (line != victim_line && line->valid && line->lru < old_lru) {
                line->lru++;
            }
        }
    }
    
    // Load data from memory into cache line
    for (int i = 0; i < BLOCK_SIZE; i++) {
        uint16_t block_addr = block_address + i;
        victim_line->data[i] = readFromMemory(dram, block_addr);
    }
    
    // Output cache state for UI visualization (for the block, not just this instruction)
    printf("[CACHE]%u:%u:%u:%u\n", set_index, 0, 1, tag);
    
    // Output all block data for UI
    for (int i = 0; i < BLOCK_SIZE; i++) {
        printf("[CACHE_DATA]%u:%u:%u:%u\n", 
               set_index, 0, i, victim_line->data[i]);
    }
    
    // Return the requested data
    return victim_line->data[block_offset];
}

/**
 * @brief Clears the cache by setting all of the line data to 0
 * @param cache The cache to be cleared
 */
void clear_cache(Cache *cache) {
  if (!cache) return;
  
  for (uint16_t i = 0; i < cache->num_sets; i++) {
    Set *set = &cache->sets[i];
    for (uint16_t j = 0; j < cache->mode; j++) {
      Line *line = &set->lines[j];
      line->valid = 0;
      line->tag = 0;
      line->lru = j;
      
      for (uint16_t k = 0; k < BLOCK_SIZE; k++) {
        line->data[k] = 0;
      }
    }
  }
}

/**
 * @brief Frees all of the associated with a cache, including the sets array, lines array, and the cache struct itself
 * @param cache The cache to be destroyed
 */
void destroy_cache(Cache *cache) {
  if (!cache) return;
  
  for (uint16_t i = 0; i < cache->num_sets; i++) {
    free(cache->sets[i].lines);
  }
  
  free(cache->sets);
  free(cache);
}
