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
    int max_cycles = 20; // Maximum cycles to run
    
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
        printf("[CYCLE] %d\n", cycles);
        fflush(stdout);
    }
    
    // Print final register values in format expected by UI
    for (int i = 0; i < 16; i++) {
        printf("[REG]%d:%d\n", i, registers->R[i]);
    }
    
    // Print cache contents for UI
    printf("[LOG] Printing cache contents\n");
    for (int i = 0; i < cache->num_sets; i++) {
        Set* set = &cache->sets[i];
        for (int j = 0; j < set->associativity; j++) {
            Line* line = &set->lines[j];
            // For each cache line, report its state to the UI
            // Format: [CACHE]index:offset:valid:data
            printf("[CACHE]%d:%d:%d:%d\n", 
                   i,                    // Index (set number)
                   j,                    // Offset (line in set)
                   line->valid,          // Valid bit (0 or 1)
                   line->tag);           // Tag value
            
            // Also output the data values in the cache line
            for (int k = 0; k < BLOCK_SIZE; k++) {
                printf("[CACHE_DATA]%d:%d:%d:%d\n", 
                       i, j, k, line->data[k]);
            }
        }
    }
    
    // Print memory values for the UI
    for (int i = 0; i < DRAM_SIZE; i++) {
        if (dram.memory[i] != 0) {
            printf("[MEM]%d:%d\n", i, dram.memory[i]);
        }
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
    
    // More detailed logging of the instruction
    printf("[DEBUG] Parsing instruction: '%s'\n", instruction);
    
    uint16_t value = loadInstruction(instruction);
    
    // Print the binary representation for better debugging
    printf("[DEBUG] Binary: ");
    for (int i = 15; i >= 0; i--) {
        printf("%u", (value >> i) & 1);
        if (i % 4 == 0 && i != 0) printf(" ");
    }
    printf("\n");
    
    // Extract opcode (top 4 bits)
    uint16_t opcode = (value >> 12) & 0xF;
    uint16_t rd = (value >> 8) & 0xF;
    uint16_t ra = (value >> 4) & 0xF;
    uint16_t rb_imm = value & 0xF;
    
    printf("[DEBUG] Decoded: opcode=%u, rd=%u, ra=%u, rb/imm=%u\n", 
            opcode, rd, ra, rb_imm);
    
    // Store in instruction space (0-499)
    uint16_t addr = registers->R[15];
    writeToMemory(&dram, addr, value);

    printf("[BIN]%u\n", value);
    printf("[MEM]%d:%d\n", addr, value);
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

        if (strncmp(command, "write", 5) == 0) {
            // Check for LW/SW special syntax with brackets and handle them
            storeInstruction(command);
        }
        else if (strncmp(command, "start", 5) == 0) executeInstructions();
        else if (strncmp(command, "break", 5) == 0) breakpointInstrcutions();
        else if (strncmp(command, "step", 4) == 0) stepInstructions();
        else {
            printf("[DEBUG] Unknown command: %s\n", command);
        }
        
        fflush(stdout);
    }

    return 0;
}

