// gcc loop.c memory.c utilities.c -o ARCH-16 -lregex

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "utilities.h"
#include "memory.h"
#include <regex.h>

#define MAX_INPUT 100

void simulationLoop(DRAM *dram, uint16_t *clockCycle, Queue *q){
  int16_t command = -1;
  char input[MAX_INPUT];
  char cmd[3] = "";
  uint16_t addr;
  int16_t value = 0;
  cmdElement element;

  regex_t regex;
  const char *pattern = "^[a-zA-Z]+(\\s[0-9]+)?(\\s[0-9]+)?$";
  regcomp(&regex, pattern, REG_EXTENDED);

  ReturnBuffer returnBuffer;
  initReturnBuffer(&returnBuffer);

  while (true) {
    // Clear the entire screen
    // printf("\033[2J\033[H");

    // Print ASCII header
    printf("=================================================================\n\n");
    printf("\n"
             " ______    ______    ______    __  __               __    ____    \n"
             "/\\  _  \\  /\\  _  \\  /\\  _  \\  /\\ \\/\\ \\             /  \\  / ___\\   \n"
             "\\ \\ \\_\\ \\ \\ \\ \\_\\ \\ \\ \\ \\/\\_\\ \\ \\ \\_\\ \\           /\\_  \\/\\ \\__/   \n"
             " \\ \\  __ \\ \\ \\    /  \\ \\ \\/_/_ \\ \\  _  \\   _______\\/_/\\ \\ \\  _``\\ \n"
             "  \\ \\ \\/\\ \\ \\ \\ \\\\ \\  \\ \\ \\_\\ \\ \\ \\ \\ \\ \\ /\\______\\  \\ \\ \\ \\ \\_\\ \\\n"
             "   \\ \\_\\ \\_\\ \\ \\_\\ \\_\\ \\ \\____/  \\ \\_\\ \\_\\ /______/   \\ \\_\\ \\____/\n"
             "    \\/_/\\/_/  \\/_/\\/_/  \\/___/    \\/_/\\/_/             \\/_/\\/___/ \n"
             "                                                                  \n"
             "                                                                  \n"
    );

    // Print current queue
    displayQueue(q);

    // Print previous command returns
    displayCommandReturns(&returnBuffer);

    // Prompt for input
    printf("\n\nARCH-16> ");
    fflush(stdout);

    fgets(input, MAX_INPUT, stdin);
    input[strcspn(input, "\n")] = '\0';

    if (!regexec(&regex, input, 0, NULL, 0)) {
      sscanf(input, "%2s %hu %hd", cmd, &addr, &value);
    } else {
      addCommandReturn(&returnBuffer, "Invalid input format.");
      continue;
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
      addCommandReturn(&returnBuffer, "Not a valid input.");
      continue;
    }

    switch (command) {
      case 0: // SW
        strcpy(element.cmd, cmd);
        element.execute = *clockCycle + 3;
        element.addr = addr;
        element.value = value;
        enqueue(q, &element);
        break;

      case 1: // SR
        strcpy(element.cmd, cmd);
        element.execute = *clockCycle + 3;
        element.addr = addr;
        enqueue(q, &element);
        break;

      case 2: // V
        strcpy(element.cmd, cmd);
        element.execute = *clockCycle + 3;
        element.addr = addr;
        enqueue(q, &element);
        break;

      case 3: // C
        clearMemory(dram);
        {
          char message[128];
          snprintf(message, sizeof(message), "Cycle: %d. Cleared Memory.", *clockCycle);
          addCommandReturn(&returnBuffer, message);
        }
        break;
    }

    // Execute commands from queue if ready
    if (checkQueue(q, clockCycle)){
      cmdElement curr = q->items[q->front];
      if (strcmp(curr.cmd, "SW") == 0){
        addCommandReturn(&returnBuffer, "Done, wrote to memory.");
        writeToMemory(dram, curr.addr, curr.value);
      } else if (strcmp(curr.cmd, "SR") == 0){
        int16_t readValue = readFromMemory(dram, curr.addr);
        char message[128];
        snprintf(message, sizeof(message), "Done, memory at address [%d] is: %d.", curr.addr, readValue);
        addCommandReturn(&returnBuffer, message);
      } else if (strcmp(curr.cmd, "V") == 0){
        char memBlock[128];
        viewBlockMemory(dram, curr.addr, memBlock);
        printf("Memory Block: %s\n", memBlock);
      }
      dequeue(q);
    }
    addCommandReturn(&returnBuffer, "Wait.");
    (*clockCycle)++;
  }
}

int main(){
  DRAM dram = {0};
  uint16_t clockCycle = 0;
  Queue q;
  initQueue(&q);
  clearMemory(&dram);

  simulationLoop(&dram, &clockCycle, &q);
  return 0;
}

