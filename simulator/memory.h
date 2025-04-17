#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define DRAM_SIZE 1000
#define INSTR_SPACE 0
#define DRAM_DELAY 1       // Delay cycles for DRAM operations set to 1
#define MAX_MEM_VIEW 16
#define MAX_VALUE_LENGTH 128
#define CMD_SIZE 7           // For commands like "SW" or "LW"

#define CACHE_SIZE 64
#define BLOCK_SIZE 4

typedef struct Cache Cache;
typedef struct Set Set;
typedef struct Line Line;

typedef struct {
    uint16_t R[16];  // Array of 16 registers (R0-R12, LR, SR, PC)
    // R[13] = LR (Link Register)
    // R[14] = SR (Status Register)
    // R[15] = PC (Program Counter)
} REGISTERS;


// DRAM state enum.
typedef enum {
    DRAM_IDLE,
    DRAM_READ,
    DRAM_WRITE
} DRAMState;

// Updated DRAM structure.
typedef struct {
    uint16_t memory[DRAM_SIZE];
    DRAMState state;
    uint16_t delayCounter;
    uint16_t pendingAddr;
    int16_t pendingValue;
    char pendingCmd[CMD_SIZE];
} DRAM;

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

REGISTERS *init_registers();

void writeToMemory(DRAM *dram, uint16_t addr, int16_t data);
uint16_t readFromMemory(DRAM *dram, uint16_t addr);
void clearMemory(DRAM *dram);
void viewBlockMemory(DRAM *dram, uint16_t addr, uint16_t numBlocks, char values[]);
void updateDRAM(DRAM *dram, Cache *cache);

// Unified memory access functions
uint16_t memory_read(Cache *cache, DRAM *dram, uint16_t address);
void memory_write(Cache *cache, DRAM *dram, uint16_t address, uint16_t data);

int write_through(Cache *cache, DRAM *dram, uint16_t address, uint16_t data);
Cache *init_cache(uint16_t mode);
Set *init_set(uint16_t mode);
Line *init_line();
void clear_cache(Cache *cache);
void destroy_cache(Cache *cache);
uint16_t read_cache(Cache *cache, DRAM *dram, uint16_t address);

#endif
