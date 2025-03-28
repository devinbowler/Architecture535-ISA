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

void start() {
  registers->PC = 0;
  memset(&pipeline, 0, sizeof(pipeline))
  printf("[LOG] Pipeline started at PC=0\n");
}

void step() {
  pipeline_step(&pipeline);
  printf("[STEP] Cycle Completed.\n");
}

void status() {
    if (pipeline.fetch.valid) {
        printf("[FETCH] raw=0x%04X pc=%d\n", pipeline.fetch.raw, pipeline.fetch.pc);
    } else {
        printf("[FETCH] (empty)\n");
    }
    printf("[END]\n");
}

void storeInstruction(const char *command) {
    const char *instruction = command + 6;
    uint16_t value = loadInstruction(instruction);

    writeToMemory(&dram, registers->PC, value);

    printf("[BIN]%u\n", value);
    printf("[MEM]%d:%d\n", registers->PC, value);
    printf("[END]\n");
    fflush(stdout);

    registers->PC++;
}


int main() {
    init_system();

    char command[256];

    while (fgets(command, sizeof(command), stdin)) {
        command[strcspn(command, "\n")] = 0;

        if (strncmp(command, "write", 5) == 0) storeInstruction(command);
        else if (strncmp(command, "start", 5) == 0) start();
        else if (strncmp(command, "step", 4) == 0) step();
        else if (strncmp(command, "status", 6) == 0) status();
    }

    return 0;
}

