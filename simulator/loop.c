#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include "utilities.h"
#include "memory.h"

#define MAX_INPUT 100
uint16_t cache_enabled = 1;

// DRAM update: if DRAM is busy, decrement the delay counter and add "wait".
// When the counter reaches 0, perform the requested access and add "done".
void updateDRAM(DRAM *dram, ReturnBuffer *rb) {
    if (dram->state != DRAM_IDLE) {
        // Decrement delay counter and respond with "wait".
        if (dram->delayCounter > 0) {
            dram->delayCounter--;
            addCommandReturn(rb, "Wait.");
        }
        // When delay reaches 0, finish the access.
        if (dram->delayCounter == 0) {
            // Check for valid address.
            if (dram->pendingAddr >= DRAM_SIZE) {
                addCommandReturn(rb, "Error: address out-of-range.");
            } else {
                if (strcmp(dram->pendingCmd, "SW") == 0) {
                    writeToMemory(dram, dram->pendingAddr, dram->pendingValue);
                    addCommandReturn(rb, "Done.");
                } else if (strcmp(dram->pendingCmd, "LW") == 0) {
                    int16_t readValue = readFromMemory(dram, dram->pendingAddr);
                    char message[128];
                    snprintf(message, sizeof(message), "Read Done. Value : [ %d ].", readValue);
                    addCommandReturn(rb, message);
                } else {
                    addCommandReturn(rb, "Error: unknown DRAM command.");
                }
            }
            // Reset DRAM state.
            dram->state = DRAM_IDLE;
            dram->pendingAddr = 0;
            dram->pendingValue = 0;
            strcpy(dram->pendingCmd, "");
        }
    }
}

void simulationLoop(DRAM *dram, Queue *q) {
    char input[MAX_INPUT];
    char cmd[CMD_SIZE] = "";
    char stage[STAGE_SIZE] = "";
    uint16_t addr = 0;
    int16_t value = 0;
    cmdElement element;
    regex_t regex;
    const char *pattern = "^[a-zA-Z]+(\\s[a-zA-Z]+)?(\\s[0-9]+)?(\\s[0-9]+)?$";
    regcomp(&regex, pattern, REG_EXTENDED);
>>>>>>> dram

    ReturnBuffer returnBuffer;
    initReturnBuffer(&returnBuffer);

    while (true) {
        // --- Display the UI.
        printf("\033[2J\033[H");
        printf("=================================================================\n\n");
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

        // --- Validate and parse input.
        if (!regexec(&regex, input, 0, NULL, 0)) {
            // For VMEM/VCACHE, no stage is required.
            if (strncmp(input, "VMEM", 4) == 0 || strncmp(input, "VCACHE", 6) == 0) {
                sscanf(input, "%s %hu %hd", cmd, &addr, &value);
                stage[0] = '\0';
            } else {
                sscanf(input, "%s %2s %hu %hd", cmd, stage, &addr, &value);
            }
        } else {
            addCommandReturn(&returnBuffer, "Invalid input format.");
            cmd[0] = '\0'; stage[0] = '\0'; addr = 0; value = 0;
            continue;
        }

        // --- Check if the command is allowed.
        if (strcmp(cmd, "SW") != 0 &&
            strcmp(cmd, "LW") != 0 &&
            strcmp(cmd, "VMEM") != 0 &&
            strcmp(cmd, "VCACHE") != 0 &&
            strcmp(cmd, "Clear") != 0 &&
            strcmp(cmd, "CACHE") != 0) {
            addCommandReturn(&returnBuffer, "Invalid command.");
            cmd[0] = '\0'; stage[0] = '\0'; addr = 0; value = 0;
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
            viewBlockMemory(dram, element.addr, element.value, memBlock);
            addCommandReturn(&returnBuffer, memBlock);
        } else if (strcmp(element.cmd, "Clear") == 0) {
            clearMemory(dram);
            addCommandReturn(&returnBuffer, "Memory cleared.");
        } else if (strcmp(element.cmd, "CACHE") == 0) {
            addCommandReturn(&returnBuffer, "CACHE command executed.");
        }
        // --- Process queued commands: SW, LW, VCACHE.
        else {
            if (!isEmpty(q)) {
                // There is an active command.
                if (cmdElementsEqual(&element, &q->items[q->front])) {
                    // The input matches the active (front) command.
                    // Process it: if DRAM is idle, load the active command.
                    if (dram->state == DRAM_IDLE) {
                        cmdElement curr = q->items[q->front];
                        if (strcmp(curr.cmd, "SW") == 0) {
                            dram->state = DRAM_WRITE;
                            dram->delayCounter = DRAM_DELAY;
                            dram->pendingAddr = curr.addr;
                            dram->pendingValue = curr.value;
                            strcpy(dram->pendingCmd, curr.cmd);
                        } else if (strcmp(curr.cmd, "LW") == 0) {
                            dram->state = DRAM_READ;
                            dram->delayCounter = DRAM_DELAY;
                            dram->pendingAddr = curr.addr;
                            strcpy(dram->pendingCmd, curr.cmd);
                        }
                    }
                    updateDRAM(dram, &returnBuffer);
                    // Only dequeue if the active command completes.
                    if (dram->state == DRAM_IDLE) {
                        dequeue(q);
                    }
                } else {
                    // The input does NOT match the active command.
                    // Check if this exact command (cmd, stage, addr, value) is already in queue (excluding active).
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
                        // Enqueue the new command.
                        enqueue(q, &element);
                        addCommandReturn(&returnBuffer, "Added to Queue.");
                    }
                }
            } else {
                // No active command; simply enqueue.
                enqueue(q, &element);
                addCommandReturn(&returnBuffer, "Added to Queue.");
            }
        }

        // --- Reset temporary variables for the next iteration.
        cmd[0] = '\0';
        stage[0] = '\0';
        addr = 0;
        value = 0;
    }
    regfree(&regex);
}


int main(){
    // Initialize DRAM.
    DRAM dram;
    clearMemory(&dram);
    Cache *cache = init_cache(1);
    dram.state = DRAM_IDLE;
    dram.delayCounter = 0;
    dram.pendingAddr = 0;
    dram.pendingValue = 0;
    strcpy(dram.pendingCmd, "");

    // Initialize the command queue.
    Queue q;
    initQueue(&q);

    simulationLoop(&dram, cache, &q);
    return 0;
}
