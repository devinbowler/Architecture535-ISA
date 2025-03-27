#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "memory.h"
#include "../assembler/assembler.h"

DRAM dram;
REGISTERS *registers;
Cache *cache;

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
    setvbuf(stdout, NULL, _IONBF, 0);  // Disable buffering
    init_system();

    char command[256];

    while (fgets(command, sizeof(command), stdin)) {
        command[strcspn(command, "\n")] = 0;
        if (strncmp(command, "write", 5) == 0) {
            storeInstruction(command);
        }
    }

    return 0;
}

