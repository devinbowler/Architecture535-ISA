
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include <ctype.h> 
#include "utilities.h"
#include "memory.h"

#define MAX_INPUT 100
uint16_t cache_enabled = 1;

void simulationLoop(DRAM *dram, Cache *cache, Queue *q) {
    char input[MAX_INPUT];
    char cmd[CMD_SIZE] = "";
    char stage[STAGE_SIZE] = "";
    uint16_t addr = 0;
    int16_t value = 0;
    cmdElement element;
    regex_t regex;
    const char *pattern = "^[a-zA-Z]+(\\s[a-zA-Z]+)?(\\s[0-9]+)?(\\s[0-9]+)?$";
    regcomp(&regex, pattern, REG_EXTENDED);

    ReturnBuffer returnBuffer;
    initReturnBuffer(&returnBuffer);

    while (true) {
        // --- Display the UI.
        printf("\033[2J\033[H");
        printf("=================================================================\n\n");
        printf("---------------- SW : Store Word | LW: Load Word ----------------\n");
        printf("------------ VMEM : View Memory | VCACHE : View Cache -----------\n");
        printf("---------------------- Clear : Clear Memory ---------------------\n");
        printf("\n"
               " ______    ______    ______    __  __               __    ____    \n"
               "/\\  _  \\  /\\  _  \\  /\\  _  \\  /\\ \\/\\ \\             /  \\  / ___\\   \n"
               "\\ \\ \\_\\ \\ \\ \\ \\_\\ \\ \\ \\ \\/\\_\\ \\ \\ \\_\\ \\           /\\_  \\/\\ \\__/   \n"
               " \\ \\  __ \\ \\ \\    /  \\ \\ \\/_/_ \\ \\  _  \\   _______\\/_/\\ \\ \\  _``\\ \n"
               "  \\ \\ \\/\\ \\ \\ \\ \\\\ \\  \\ \\ \\_\\ \\ \\ \\ \\ \\ \\ /\\______\\  \\ \\ \\ \\ \\_\\ \n"
               "   \\ \\_\\ \\_\\ \\ \\_\\ \\_\\ \\ \\____/  \\ \\_\\ \\_\\ /______/   \\ \\_\\ \\____/\n"
               "    \\/_/\\/_/  \\/_/\\/_/  \\/___/    \\/_/\\/_/             \\/_/\\/___/ \n"
               "                                                                  \n"
               "                                                                  \n");
        displayQueue(q);
        displayCommandReturns(&returnBuffer);
        // --- Prompt for user input.
        printf("\n\nARCH-16> ");
        fflush(stdout);
        if (fgets(input, MAX_INPUT, stdin) == NULL)
            break;
        input[strcspn(input, "\n")] = '\0';

        // --- Validate input (basic regex check).
        if (regexec(&regex, input, 0, NULL, 0) != 0) {
            addCommandReturn(&returnBuffer, "Invalid input format.");
            continue;
        }

        // Extract the command token.
        if (sscanf(input, "%s", cmd) != 1) {
            addCommandReturn(&returnBuffer, "Invalid input.");
            continue;
        }

        // --- Parse input based on the command.
        if (strcmp(cmd, "VMEM") == 0 || strcmp(cmd, "VCACHE") == 0) {
            // Expect exactly two tokens: command and address.
            int tokens = sscanf(input, "%s %hu", cmd, &addr);
            if (tokens != 2) {
                addCommandReturn(&returnBuffer, "Invalid input format for VMEM/VCACHE.");
                continue;
            }
            stage[0] = '\0';
            value = 0;
        } else if (strcmp(cmd, "LW") == 0) {
            // Expect exactly three tokens: command, stage, address.
            int tokens = sscanf(input, "%s %2s %hu", cmd, stage, &addr);
            if (tokens != 3) {
                addCommandReturn(&returnBuffer, "Invalid input format for LW.");
                continue;
            }
            value = 0;  // For LW, value is not provided.
            if (!isalpha(stage[0])) {
                addCommandReturn(&returnBuffer, "Invalid stage for LW.");
                continue;
            }
        } else if (strcmp(cmd, "SW") == 0) {
            // Expect exactly four tokens: command, stage, address, and value.
            int tokens = sscanf(input, "%s %2s %hu %hd", cmd, stage, &addr, &value);
            if (tokens != 4) {
                addCommandReturn(&returnBuffer, "Invalid input format for SW.");
                continue;
            }
            if (!isalpha(stage[0])) {
                addCommandReturn(&returnBuffer, "Invalid stage for SW.");
                continue;
            }
        } else if (strcmp(cmd, "Clear") == 0) {
            // No additional tokens needed.
        } else {
            addCommandReturn(&returnBuffer, "Invalid command.");
            continue;
        }

        // --- Prepare the command element.
        strcpy(element.cmd, cmd);
        strcpy(element.stage, stage);
        element.addr = addr;
        element.value = value;

        // --- Process instant commands.
        if (strcmp(element.cmd, "VMEM") == 0) {
            char memBlock[128];
            viewBlockMemory(dram, element.addr, 1, memBlock);
            addCommandReturn(&returnBuffer, memBlock);
        } else if (strcmp(element.cmd, "VCACHE") == 0) {
            if (cache == NULL) {
                addCommandReturn(&returnBuffer, "Cache disabled.");
            } else {
                uint16_t index = (addr / BLOCK_SIZE) % cache->num_sets;
                uint16_t tag = addr / (BLOCK_SIZE * cache->num_sets);
                uint16_t offset = addr % BLOCK_SIZE;
                Set *set = &cache->sets[index];
                // Direct mapped: one line per set.
                Line *line = &set->lines[0];
                char buffer[256];
                snprintf(buffer, sizeof(buffer),
                         "Cache Line Info:\nIndex: %d\nTag: %d\nOffset: %d\nValid: %d\nData: %d %d %d %d",
                         index, tag, offset, line->valid,
                         line->data[0], line->data[1], line->data[2], line->data[3]);
                addCommandReturn(&returnBuffer, buffer);
            }
        } else if (strcmp(element.cmd, "Clear") == 0) {
            clearMemory(dram);
            addCommandReturn(&returnBuffer, "Memory cleared.");
        }         
        // --- Process queued commands: SW and LW.
        else {
            if (isEmpty(q)) {
                enqueue(q, &element);
                addCommandReturn(&returnBuffer, "Added to Queue.");
            } else {
                cmdElement curr = q->items[q->front];
                if (cmdElementsEqual(&element, &curr)) {
                    // The new input exactly matches the active command.
                    if (strcmp(curr.cmd, "LW") == 0) {
                        if (cache != NULL) {
                            uint16_t index = (curr.addr / BLOCK_SIZE) % cache->num_sets;
                            uint16_t tag = curr.addr / (BLOCK_SIZE * cache->num_sets);
                            Set *set = &cache->sets[index];
                            Line *line = &set->lines[0];
                            if (line->valid && line->tag == tag) {
                                // Cache hit: immediately return cached value.
                                char message[128];
                                snprintf(message, sizeof(message), "done %d", line->data[curr.addr % BLOCK_SIZE]);
                                addCommandReturn(&returnBuffer, message);
                                dequeue(q);
                                goto loop_end;
                            } else {
                                // Cache miss: if DRAM idle, initialize DRAM read.
                                if (dram->state == DRAM_IDLE) {
                                    dram->state = DRAM_READ;
                                    dram->delayCounter = DRAM_DELAY;
                                    dram->pendingAddr = curr.addr;
                                    strcpy(dram->pendingCmd, curr.cmd);
                                }
                            }
                        } else {
                            if (dram->state == DRAM_IDLE) {
                                dram->state = DRAM_READ;
                                dram->delayCounter = DRAM_DELAY;
                                dram->pendingAddr = curr.addr;
                                strcpy(dram->pendingCmd, curr.cmd);
                            }
                        }
                    } else if (strcmp(curr.cmd, "SW") == 0) {
                        if (cache != NULL) {
                            uint16_t index = (curr.addr / BLOCK_SIZE) % cache->num_sets;
                            uint16_t tag = curr.addr / (BLOCK_SIZE * cache->num_sets);
                            Set *set = &cache->sets[index];
                            Line *line = &set->lines[0];
                            if (line->valid && line->tag == tag) {
                                // Write hit: update the cache line with the new value.
                                uint16_t offset = curr.addr % BLOCK_SIZE;
                                line->data[offset] = curr.value;
                                addCommandReturn(&returnBuffer, "Wrote to Cache.");
                            }
                        }
                        if (dram->state == DRAM_IDLE) {
                            // Initialize DRAM write (write-through).
                            dram->state = DRAM_WRITE;
                            dram->delayCounter = DRAM_DELAY;
                            dram->pendingAddr = curr.addr;
                            dram->pendingValue = curr.value;
                            strcpy(dram->pendingCmd, curr.cmd);
                        }
                    }
                    // Call updateDRAM so that the delay counter is decremented.
                    updateDRAM(dram, cache, &returnBuffer);
                    if (dram->state == DRAM_IDLE) {
                        dequeue(q);
                    }
                } else {
                    // New command does not match active one.
                    bool exists = false;
                    for (int i = q->front + 1; i < q->rear; i++) {
                        if (cmdElementsEqual(&element, &q->items[i])) {
                            exists = true;
                            break;
                        }
                    }
                    if (exists) {
                        addCommandReturn(&returnBuffer, "Command already in queue. Wait for completion.");
                    } else {
                        enqueue(q, &element);
                        addCommandReturn(&returnBuffer, "Added to Queue.");
                    }
                }
            }
        }
    loop_end:
        // Reset temporary variables for the next iteration.
        cmd[0] = '\0';
        stage[0] = '\0';
        addr = 0;
        value = 0;
    }
    regfree(&regex);
}

int main(int argc, char *argv[]){
  // If no argument is provided, default to mode 1 (direct mapped cache).
  uint16_t mode = 1;
  if(argc >= 2) {
    mode = atoi(argv[1]);
  }
  
  // Initialize DRAM.
  DRAM dram;
  clearMemory(&dram);
  dram.state = DRAM_IDLE;
  dram.delayCounter = 0;
  dram.pendingAddr = 0;
  dram.pendingValue = 0;
  strcpy(dram.pendingCmd, "");
  
  // Set up the cache pointer.
  Cache *cache = NULL;
  if (mode != 0) {  // mode 0 means "no cache"
    // For now, we only support direct mapped, which is mode 1.
    cache = init_cache(1);
    if (!cache) {
      fprintf(stderr, "Error initializing cache\n");
      return 1;
    }
  }
  
  // Initialize the command queue.
  Queue q;
  initQueue(&q);
  
  simulationLoop(&dram, cache, &q);
  return 0;
}

