#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "memory.h"
#include "pipeline.h"
#include "../assembler/assembler.h"


DRAM dram;
REGISTERS *registers;
Cache *cache;
PipelineState pipeline;

// Initialize all systems.
void init_system() {
    clearMemory(&dram);
    dram.state = DRAM_IDLE;
    dram.delayCounter = 0;
    dram.pendingAddr = 0;
    dram.pendingValue = 0;
    strcpy(dram.pendingCmd, "");

    registers = init_registers();
    cache = init_cache(1);

    printf("[LOG] System is Initialized\n");
    fflush(stdout);
}

// Execute all instructions in DRAM.
void executeInstructions() {
    registers->R[15] = 0;
    memset(&pipeline, 0, sizeof(pipeline));  // First clear everything
    pipeline.IF_ID.valid = true;  // Start with IF_ID empty so fetch can proceed
    pipeline.ID_EX.valid = true;  // Start with ID_EX empty
    printf("[LOG] Pipeline started at PC=0\n");
    fflush(stdout);

    uint16_t instruction = readFromMemory(&dram, registers->R[15]);
    int cycles = 0;
    int max_cycles = 7; // Fixed cycle count for 2 instructions (5 stages + 2 for setup)

    while (cycles < max_cycles) {
        // Fetch next instruction if available
        if (instruction != 0) {
            pipeline_step(&pipeline, &instruction);
            registers->R[15] += 1;
            instruction = readFromMemory(&dram, registers->R[15]);
        } else {
            // No more instructions to fetch, but keep pipeline running
            pipeline_step(&pipeline, &instruction);
        }
        cycles++;
    }
    
    // Print final register values in format expected by UI
    for (int i = 0; i < 16; i++) {
        printf("[REG]%d:%d\n", i, registers->R[i]);
    }
    
    printf("[END]\n");
    fflush(stdout);
}

// Exectue instructions in DRAM until specified breakpoint.
void breakpointInstrcutions() {
    printf("[BREAKPOINT] Execute to Breakpoint.\n");
    fflush(stdout);
}

// Step through instructions one by one in DRAM.
void stepInstructions() {
  // pipeline_step(&pipeline);
  printf("[STEP] Cycle Completed.\n");
  fflush(stdout);
}

// Store all instruction to DRAM at current PC address.
void storeInstruction(const char *command) {
    const char *instruction = command + 6;
    uint16_t value = loadInstruction(instruction);

    writeToMemory(&dram, registers->R[15], value);

    printf("[BIN]%u\n", value);
    printf("[MEM]%d:%d\n", registers->R[15], value);
    printf("[END]\n");
    fflush(stdout);

    registers->R[15] += 1;
}


int main() {
    init_system();

    char command[256];

    while (fgets(command, sizeof(command), stdin)) {
        command[strcspn(command, "\n")] = 0;
        printf("[DEBUG] Received command: %s\n", command);
        fflush(stdout);

        if (strncmp(command, "write", 5) == 0) storeInstruction(command);
        else if (strncmp(command, "start", 5) == 0) executeInstructions();
        else if (strncmp(command, "break", 5) == 0) breakpointInstrcutions();
        else if (strncmp(command, "step", 4) == 0) stepInstructions();
    }

    return 0;
}

