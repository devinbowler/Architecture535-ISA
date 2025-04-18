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

// --- stepping‑state globals for stepInstructions() ---
static uint16_t step_instr_val = 0;
static int      step_cycle_cnt = 0;
static bool     step_init      = false;

// Global Variables
REGISTERS    *registers;
DRAM          dram;
Cache        *cache;
PipelineState pipeline;

// Initialize all systems.
void init_system() {
    clearMemory(&dram);
    dram.state        = DRAM_IDLE;
    dram.delayCounter = 0;
    dram.pendingAddr  = 0;
    dram.pendingValue = 0;
    strcpy(dram.pendingCmd, "");

    registers = init_registers();
    cache     = init_cache(1);

    // reset stepping state
    step_init      = false;
    step_cycle_cnt = 0;
    step_instr_val = 0;

    printf("[LOG] System is Initialized\n");
    fflush(stdout);
}

// Execute all instructions in DRAM.
void executeInstructions() {
    registers->R[15] = 0;
    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.IF_ID.valid  = false;
    pipeline.ID_EX.valid  = false;
    pipeline.EX_MEM.valid = false;
    pipeline.MEM_WB.valid = false;
    pipeline.WB.valid     = false;

    printf("[LOG] Pipeline started at PC=0\n");
    fflush(stdout);

    uint16_t instruction = readFromMemory(&dram, registers->R[15]);
    int cycles = 0, max_cycles = 100;

    while (cycles < max_cycles) {
        if (instruction != 0) {
            pipeline_step(&pipeline, &instruction);
            instruction = readFromMemory(&dram, registers->R[15]);
        } else {
            pipeline_step(&pipeline, &instruction);
        }

        cycles++;
        printf("[CYCLE] %d\n", cycles);
        fflush(stdout);
    }

    // final dump
    for (int i = 0; i < 16; i++)
        printf("[REG]%d:%d\n", i, registers->R[i]);

    printf("[LOG] Printing cache contents\n");
    for (int i = 0; i < cache->num_sets; i++) {
        Set *set = &cache->sets[i];
        for (int j = 0; j < set->associativity; j++) {
            Line *line = &set->lines[j];
            printf("[CACHE]%d:%d:%d:%d\n", i, j, line->valid, line->tag);
            for (int k = 0; k < BLOCK_SIZE; k++)
                printf("[CACHE_DATA]%d:%d:%d:%d\n", i, j, k, line->data[k]);
        }
    }

    for (int i = 0; i < DRAM_SIZE; i++)
        if (dram.memory[i] != 0)
            printf("[MEM]%d:%d\n", i, dram.memory[i]);

    printf("[END]\n");
    fflush(stdout);
}

// Step through instructions one by one.
void stepInstructions() {
    if (!step_init) {
        registers->R[15] = 0;
        memset(&pipeline, 0, sizeof(pipeline));
        pipeline.IF_ID.valid  = false;
        pipeline.ID_EX.valid  = false;
        pipeline.EX_MEM.valid = false;
        pipeline.MEM_WB.valid = false;
        pipeline.WB.valid     = false;

        step_instr_val = readFromMemory(&dram, registers->R[15]);
        step_cycle_cnt = 0;
        step_init      = true;
        printf("[LOG] Pipeline initialized for stepping, PC=0\n");
    }

    if (step_instr_val != 0) {
        pipeline_step(&pipeline, &step_instr_val);
        step_instr_val = readFromMemory(&dram, registers->R[15]);
    } else {
        pipeline_step(&pipeline, &step_instr_val);
    }

    step_cycle_cnt++;
    printf("[CYCLE]%d\n", step_cycle_cnt);

    for (int i = 0; i < 16; i++)
        printf("[REG]%d:%d\n", i, registers->R[i]);

    printf("[LOG] Printing cache contents\n");
    for (int i = 0; i < cache->num_sets; i++) {
        Set *set = &cache->sets[i];
        for (int j = 0; j < set->associativity; j++) {
            Line *line = &set->lines[j];
            printf("[CACHE]%d:%d:%d:%d\n", i, j, line->valid, line->tag);
            for (int k = 0; k < BLOCK_SIZE; k++)
                printf("[CACHE_DATA]%d:%d:%d:%d\n", i, j, k, line->data[k]);
        }
    }

    for (int i = 0; i < DRAM_SIZE; i++)
        if (dram.memory[i] != 0)
            printf("[MEM]%d:%d\n", i, dram.memory[i]);

    printf("[END]\n");
    fflush(stdout);
}

// Store an instruction into DRAM at the address pointed to by PC.
void storeInstruction(const char *command) {
    const char *instrPtr = command + 6;
    uint16_t value = loadInstruction(instrPtr);

    printf("[DEBUG] Binary: ");
    for (int i = 15; i >= 0; i--) {
        printf("%u", (value >> i) & 1);
        if (i % 4 == 0 && i != 0) printf(" ");
    }
    printf("\n");

    uint16_t addr = registers->R[15];
    writeToMemory(&dram, addr, value);

    printf("[BIN]%u\n", value);
    printf("[MEM]%d:%d\n", addr, value);
    printf("[END]\n");
    fflush(stdout);

    registers->R[15]++;
}

static void apply_config(const char *params) {
    char key[32], val[32];
    while (sscanf(params, " %31[^ =]=%31s", key, val) == 2) {
        if (strcmp(key, "pipe") == 0) {
            PIPELINE_ENABLED = atoi(val) != 0;
            printf("[CONFIG] Pipeline %s\n", PIPELINE_ENABLED ? "enabled" : "disabled");
        }
        else if (strcmp(key, "cache") == 0) {
            CACHE_ENABLED = atoi(val) != 0;
            printf("[CONFIG] Cache %s\n", CACHE_ENABLED ? "enabled" : "disabled");
        }
        else if (strcmp(key, "dram") == 0) {
            USER_DRAM_DELAY = atoi(val);
            printf("[CONFIG] DRAM delay set to %u cycles\n", USER_DRAM_DELAY);
        }
        else if (strcmp(key, "cache_delay") == 0) {
            USER_CACHE_DELAY = atoi(val);
            printf("[CONFIG] Cache delay set to %u cycles\n", USER_CACHE_DELAY);
        }
        params = strchr(params, ' ');
        if (!params) break;
        ++params;
    }
}

int main() {
    init_system();
    char command[256];

    while (fgets(command, sizeof(command), stdin)) {
        command[strcspn(command, "\n")] = 0;
        printf("[DEBUG] Received command: %s\n", command);
        fflush(stdout);

        if      (strncmp(command, "write", 5) == 0) storeInstruction(command);
        else if (strncmp(command, "start", 5) == 0) executeInstructions();
        else if (strncmp(command, "step", 4)  == 0) stepInstructions();
        else if (strncmp(command, "reset", 5) == 0) {
            init_system();
            printf("[END]\n");
            fflush(stdout);
        }
        else if (strncmp(command, "cfg", 3) == 0) {
            uint16_t d, cd, ce, pe;
            if (sscanf(command + 3, " %hu %hu %hu %hu", &d, &cd, &ce, &pe) == 4) {
                USER_DRAM_DELAY   = d;
                USER_CACHE_DELAY  = cd;
                CACHE_ENABLED     = ce;
                PIPELINE_ENABLED  = pe;
            }
            printf("[END]\n");
            fflush(stdout);
        }
        else if (strncmp(command,"config",6) == 0) {
            apply_config(command+6);
            printf("[END]\n");
            fflush(stdout);
        }
        else {
            printf("[DEBUG] Unknown command: %s\n", command);
            fflush(stdout);
        }
    }
    return 0;
}
