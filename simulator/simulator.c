#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "memory.h"
#include "pipeline.h"
#include "simulator.h"
#include "../assembler/assembler.h"
#include "globals.h"

// Global Variables
REGISTERS *registers;
DRAM dram;
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

// Helper function to check if the entire pipeline is empty.
bool pipeline_empty(PipelineState* pipeline) {
    return !pipeline->IF_ID.valid && 
           !pipeline->ID_EX.valid && 
           !pipeline->EX_MEM.valid && 
           !pipeline->MEM_WB.valid && 
           !pipeline->WB.valid;
}

// Execute all instructions in DRAM.
void executeInstructions() {
    // Reset the PC and clear the entire pipeline.
    registers->R[15] = 0;
    memset(&pipeline, 0, sizeof(pipeline));
    
    // Initialize all pipeline registers as empty (bubble).
    pipeline.IF_ID.valid  = false;
    pipeline.ID_EX.valid  = false;
    pipeline.EX_MEM.valid = false;
    pipeline.MEM_WB.valid = false;
    pipeline.WB.valid     = false;
    
    printf("[LOG] Pipeline started at PC=0\n");
    fflush(stdout);

    uint16_t instruction = readFromMemory(&dram, registers->R[15]);
    int cycles = 0;
    int max_cycles = 100; // Maximum cycles as a safeguard
    
    // Continue stepping the pipeline while there is an instruction to fetch
    // or while the pipeline still contains valid instructions.
    while (cycles < max_cycles) {
        // If there is a new instruction available, fetch and update PC.
        if (instruction != 0) {
            pipeline_step(&pipeline, &instruction);
            instruction = readFromMemory(&dram, registers->R[15]);
        } else {
            // No more new instructions, continue stepping to flush the pipeline.
            pipeline_step(&pipeline, &instruction);
        }
        cycles++;
        printf("[CYCLE] %d\n", cycles);

        if (BREAKPOINT_PC >= 0 && registers->R[15] == BREAKPOINT_PC) {
            printf("[BREAK]\n");        /* UI catches this token */
            break;
        }
        fflush(stdout);
    }
    
    // Print final register values in expected UI format.
    for (int i = 0; i < 16; i++) {
        printf("[REG]%d:%d\n", i, registers->R[i]);
    }
    
    // Print cache contents.
    printf("[LOG] Printing cache contents\n");
    for (int i = 0; i < cache->num_sets; i++) {
        Set* set = &cache->sets[i];
        for (int j = 0; j < set->associativity; j++) {
            Line* line = &set->lines[j];
            printf("[CACHE]%d:%d:%d:%d\n", i, j, line->valid, line->tag);
            for (int k = 0; k < BLOCK_SIZE; k++) {
                printf("[CACHE_DATA]%d:%d:%d:%d\n", i, j, k, line->data[k]);
            }
        }
    }
    
    // Print non-zero memory values.
    for (int i = 0; i < DRAM_SIZE; i++) {
        if (dram.memory[i] != 0) {
            printf("[MEM]%d:%d\n", i, dram.memory[i]);
        }
    }
    
    printf("[END]\n");
    fflush(stdout);
}

// Execute instructions until reaching a breakpoint (stub function).
void breakpointInstrcutions() {
    printf("[BREAKPOINT] Execute to Breakpoint.\n");
    fflush(stdout);
}

// Step through instructions one by one.
void stepInstructions() {
    static uint16_t instruction = 0;
    static int cycle_count = 0;
    static bool initialized = false;
  
    if (!initialized) {
        registers->R[15] = 0;
        memset(&pipeline, 0, sizeof(pipeline));
        // Initialize all pipeline registers as empty except for starting fetch.
        pipeline.IF_ID.valid  = false;
        pipeline.ID_EX.valid  = false;
        pipeline.EX_MEM.valid = false;
        pipeline.MEM_WB.valid = false;
        pipeline.WB.valid     = false;
        instruction = readFromMemory(&dram, registers->R[15]);
        initialized = true;
        cycle_count = 0;
        printf("[LOG] Pipeline initialized for stepping, PC=0\n");
    }
  
    printf("[LOG] Executing cycle %d\n", cycle_count + 1);

  
    if (instruction != 0) {
        pipeline_step(&pipeline, &instruction);
        instruction = readFromMemory(&dram, registers->R[15]);
    } else {
        pipeline_step(&pipeline, &instruction);
    }
    cycle_count++;
  
    // Print current register and cache states for debugging.
    for (int i = 0; i < 16; i++) {
        printf("[REG]%d:%d\n", i, registers->R[i]);
    }
    printf("[LOG] Printing cache contents\n");
    for (int i = 0; i < cache->num_sets; i++) {
        Set* set = &cache->sets[i];
        for (int j = 0; j < set->associativity; j++) {
            Line* line = &set->lines[j];
            printf("[CACHE]%d:%d:%d:%d\n", i, j, line->valid, line->tag);
            for (int k = 0; k < BLOCK_SIZE; k++) {
                printf("[CACHE_DATA]%d:%d:%d:%d\n", i, j, k, line->data[k]);
            }
        }
    }
  
    for (int i = 0; i < DRAM_SIZE; i++) {
        if (dram.memory[i] != 0) {
            printf("[MEM]%d:%d\n", i, dram.memory[i]);
        }
    }
  
    printf("[CYCLE]%d\n", cycle_count);
    printf("[END]\n");
    fflush(stdout);
}

// Store an instruction into DRAM at the address pointed to by PC.
void storeInstruction(const char *command) {
    const char *instrPtr = command + 6;
    printf("[DEBUG] Parsing instruction: '%s'\n", instrPtr);
    uint16_t value = loadInstruction(instrPtr);

    // Print binary representation for debugging.
    printf("[DEBUG] Binary: ");
    for (int i = 15; i >= 0; i--) {
        printf("%u", (value >> i) & 1);
        if (i % 4 == 0 && i != 0) printf(" ");
    }
    printf("\n");

    uint16_t opcode  = (value >> 12) & 0xF;
    uint16_t rd      = (value >> 8) & 0xF;
    uint16_t ra      = (value >> 4) & 0xF;
    uint16_t rb_imm  = value & 0xF;
    printf("[DEBUG] Decoded: opcode=%u, rd=%u, ra=%u, rb/imm=%u\n",
           opcode, rd, ra, rb_imm);

    uint16_t addr = registers->R[15];
    writeToMemory(&dram, addr, value);

    printf("[BIN]%u\n", value);
    printf("[MEM]%d:%d\n", addr, value);
    printf("[END]\n");
    fflush(stdout);

    registers->R[15] += 1;
}


static void apply_config(const char *params)
{
    extern bool     PIPELINE_ENABLED, CACHE_ENABLED;
    extern uint16_t USER_DRAM_DELAY,  USER_CACHE_DELAY;
    extern int16_t  BREAKPOINT_PC;

    char key[32]; char val[32];
    while (sscanf(params, " %31[^ =]=%31s", key, val) == 2) {
        if      (strcmp(key,"pipe")==0) PIPELINE_ENABLED = atoi(val);
        else if (strcmp(key,"cache")==0)CACHE_ENABLED    = atoi(val);
        else if (strcmp(key,"dram")==0) USER_DRAM_DELAY  = atoi(val);
        else if (strcmp(key,"cache_delay")==0)
                                     USER_CACHE_DELAY    = atoi(val);
        else if (strcmp(key,"bp")==0)  BREAKPOINT_PC     = atoi(val);
        /* advance to next token */
        params = strchr(params,' '); if (!params) break; ++params;
    }
}

int main() {
    init_system();

    char command[256];

    while (fgets(command, sizeof(command), stdin)) {
        command[strcspn(command, "\n")] = 0;
        printf("[DEBUG] Received command: %s\n", command);
        fflush(stdout);

        if (strncmp(command, "write", 5) == 0) {
            storeInstruction(command);
        } else if (strncmp(command, "start", 5) == 0) {
            executeInstructions();
        } else if (strncmp(command, "break", 5) == 0) {
            breakpointInstrcutions();
        } else if (strncmp(command, "step", 4) == 0) {
            stepInstructions();
        } else if (strncmp(command, "reset", 5) == 0) {
            init_system();
            for (int i = 0; i < 16; i++) {
                printf("[REG]%d:%d\n", i, registers->R[i]);
            }
            for (int i = 0; i < cache->num_sets; i++) {
                Set* set = &cache->sets[i];
                for (int j = 0; j < set->associativity; j++) {
                    printf("[CACHE]%d:%d:0:0\n", i, j);
                    for (int k = 0; k < BLOCK_SIZE; k++) {
                        printf("[CACHE_DATA]%d:%d:%d:0\n", i, j, k);
                    }
                }
            }
            printf("[CYCLE]0\n");
            printf("[END]\n");
            fflush(stdout);
        } else if (strncmp(command,"cfg",3)==0){
            /* allow positional version:  dram cache pipe cacheDelay bp */
            uint16_t d,c,p,cd;  int16_t bp;
            if (sscanf(command+3," %hu %hu %hu %hu %hd",&d,&c,&p,&cd,&bp)==5){
                USER_DRAM_DELAY  = d;
                CACHE_ENABLED    = c;
                PIPELINE_ENABLED = p;
                USER_CACHE_DELAY = cd;
                BREAKPOINT_PC    = bp;
            }
            printf("[END]\n");   fflush(stdout);
        } else if (strncmp(command,"config",6)==0){
            apply_config(command+6);
            printf("[END]\n");   fflush(stdout);
        } else {
            printf("[DEBUG] Unknown command: %s\n", command);
        }
        fflush(stdout);
    }

    return 0;
}
