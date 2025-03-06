#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "memory.h"

int cycle_count = 0;
int delay = 0;

void simulationLoop(DRAM *dram){
  char input[3];
  int command;

  while (true) {
    printf("ARCH-16> ");
    scanf("%s", input);
    printf("Entered command is: %s\n", input);
    if (strcmp(input, "SW") == 0){
      command = 0;
    } else if (strcmp(input, "SR") == 0) {
      command = 1;
    } else if (strcmp(input, "V") == 0) {
      command = 2;
    } else if (strcmp(input, "C") == 0) {
      command = 3;
    } else {
      printf("Not a valid input.\n");
    }

    switch (command) {
        int addr;
        int variable;
        uint16_t value;

      case 0:
        // Write to memory.
        printf("Enter the Address.\n");
        printf("ARCH-16> ");
        scanf("%d", &addr); 
        printf("Enter the Value.\n");
        printf("ARCH-16> ");
        scanf("%d", &variable); 
        writeToMemory(dram, addr, variable);
        printf("Wrote to memory.\n");
        break;
      case 1:
        // Read from memory.
        printf("Enter the Address.\n");
        printf("ARCH-16> ");
        scanf("%d", &addr); 
        value = readFromMemory(dram, addr);

        printf("Read from address [ %d ], the value of: %d.\n", addr, value);
        break;
      case 2:
        // Show a line in memory.
        printf("Enter the Address.\n");
        printf("ARCH-16> ");
        scanf("%d", &addr); 
        char inputStr[16];
        viewRawMemory(dram, addr, inputStr);

        printf("Showing line [ %d ] in memory: %s\n", addr, inputStr);
        break;
      case 3:
        clearMemory(dram);
        printf("Cleared Memory.");
      default:
        printf("Not a valid operation.%d\n", command);
    }
  }
}

int main(){
  DRAM dram = {0};
  clearMemory(&dram); // Good practice.

  simulationLoop(&dram);
  return 0;
}

