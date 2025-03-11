#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define DRAM_SIZE 50000
#define DRAM_DELAY 2        // Delay cycles for DRAM operations set to 1
#define MAX_MEM_VIEW 16
#define MAX_VALUE_LENGTH 128
#define CMD_SIZE 7           // For commands like "SW" or "LW"

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

void writeToMemory(DRAM *dram, uint16_t addr, int16_t data);
uint16_t readFromMemory(DRAM *dram, uint16_t addr);
void clearMemory(DRAM *dram);
void viewBlockMemory(DRAM *dram, uint16_t addr, uint16_t numBlocks, char values[]);

#endif
