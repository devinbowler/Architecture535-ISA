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

void storeInstructions(){
    printf("Ready to read Instructions.\n");
    fflush(stdout);
}

int main(){
    setvbuf(stdout, NULL, _IONBF, 0);  // Disable buffering for instant output
    init_system();
  
    char command[256];

    while (fgets(command, sizeof(command), stdin)) {
        command[strcspn(command, "\n")] = 0;
        printf("Received Command: %s\n", command);
        fflush(stdout);

        if (strcmp(command, "store") == 0) {
            storeInstructions();
        }
    }

    return 0;
}
