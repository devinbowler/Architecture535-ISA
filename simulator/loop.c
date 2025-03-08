#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "memory.h"
#include <regex.h>

#define MAX_INPUT 100

void simulationLoop(DRAM *dram){
  int16_t command = 5;
  char input[MAX_INPUT];
  char cmd[3] = "";
  uint16_t addr;
  int16_t value = 0;

  regex_t regex;
  int16_t regRet;
  const char *pattern = "^[a-zA-Z]+(\\s[0-9]+)?(\\s[0-9]+)?$";
  regRet = regcomp(&regex, pattern, REG_EXTENDED);


  printf("Enter one of the following: \n");
  printf(" - Write to memory : 'SW [Addr] [Value]'.\n");
  printf(" - Read from memory : 'SR [Addr]'.\n");
  printf(" - Memory line Visual : 'V [Addr]'.\n");
  printf(" - Clear Memory : 'C'.\n");

  while (true) {
    printf("ARCH-16> ");
    fgets(input, MAX_INPUT, stdin);
    input[strcspn(input, "\n")] = '\0';

    regRet = regexec(&regex, input, 0, NULL, 0);
    if (!regRet) {
      int matches = sscanf(input, "%2s %hu %hd", cmd, &addr, &value);
    } else {
      printf("Invalid input format.");
    }

    if (strcmp(cmd, "SW") == 0){
      command = 0;
    } else if (strcmp(cmd, "SR") == 0) {
      command = 1;
    } else if (strcmp(cmd, "V") == 0) {
      command = 2;
    } else if (strcmp(cmd, "C") == 0) {
      command = 3;
    } else {
      printf("Not a valid input.\n");
    }

    switch (command) {
      case 0:
        // Write to memory.
        writeToMemory(dram, addr, value);
        printf("Wrote to memory.\n");
        break;
      case 1:
        // Read from memory.
        value = readFromMemory(dram, addr);
        printf("Read from address [ %d ], the value of: %d.\n", addr, value);
        break;
      case 2:
        // Show a line in memory.
        char inputStr[16];
        viewRawMemory(dram, addr, inputStr);
        printf("Showing line [ %d ] in memory: %s\n", addr, inputStr);
        break;
      case 3:
        clearMemory(dram);
        printf("Cleared Memory.");
      default:
        printf("Not a valid operation : %d.\n", command);
    }
  }
}

int main(){
  // Initalize DRAM and Command Queue
  DRAM dram = {0};
  // Queue q;
  clearMemory(&dram); // Good practice.


  simulationLoop(&dram);
  return 0;
}

