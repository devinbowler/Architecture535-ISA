#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "memory.h"

DRAM dram;
REGISTERS *registers;
Cache *cache;

void init_system(){
    clearMemory(&dram);
    dram.state = DRAM_IDLE;
    dram.delayCounter = 0;
    dram.pendingAddr = 0;
    dram.pendingValue = 0;
    strcpy(dram.pendingCmd, "");

    registers = init_registers();
    cache = init_cache(1);

    printf("System is Initialized\n");
    fflush(stdout);
}

void storeInstruction(uint16_t value){
    // Store the binary encoding in DRAM at the PC address.
    writeToMemory(&dram, registers->PC, value); 
    
    // Update DRAM with each of these instructions, update PC.
    printf("Stored Instruction [ %hu ] . At address [ %hu ].\n", value, registers->PC);
    fflush(stdout);
    
    registers->PC = PC++;
}

int main(){
    setvbuf(stdout, NULL, _IONBF, 0);  // Disable buffering for instant output
    init_system();
  
    char command[256];

    while (fgets(command, sizeof(command), stdin)) {
        command[strcspn(command, "\n")] = 0;
        printf("Received Command: %s\n", command);
        fflush(stdout);

        if (strcmp(command, "write", 5) == 0) {
            uint16_t value;
            if (sscanf(command, "write %hu", &value) == 1) {
              storeInstruction(value);
        }
    }

    return 0;
}
