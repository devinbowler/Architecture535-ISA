
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include "utilities.h"
#include "memory.h"

#define MAX_INPUT 100

// DRAM update: if DRAM is busy, decrement the delay counter and add "wait".
// When the counter reaches 0, perform the requested access and add "done".
void updateDRAM(DRAM *dram, ReturnBuffer *rb) {
    if (dram->state != DRAM_IDLE) {
        // Decrement delay counter and respond with "wait".
        if (dram->delayCounter > 0) {
            dram->delayCounter--;
            addCommandReturn(rb, "wait");
        }
        // When delay reaches 0, finish the access.
        if (dram->delayCounter == 0) {
            // Check for valid address.
            if (dram->pendingAddr >= DRAM_SIZE) {
                addCommandReturn(rb, "Error: address out-of-range.");
            } else {
                if (strcmp(dram->pendingCmd, "SW") == 0) {
                    writeToMemory(dram, dram->pendingAddr, dram->pendingValue);
                    addCommandReturn(rb, "done");
                } else if (strcmp(dram->pendingCmd, "LW") == 0) {
                    int16_t readValue = readFromMemory(dram, dram->pendingAddr);
                    char message[128];
                    snprintf(message, sizeof(message), "done %d", readValue);
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
    uint16_t addr;
    int16_t value = 0;
    cmdElement element;
    regex_t regex;
    const char *pattern = "^[a-zA-Z]+(\\s[a-zA-Z]+)?(\\s[0-9]+)?(\\s[0-9]+)?$";
    regcomp(&regex, pattern, REG_EXTENDED);

    ReturnBuffer returnBuffer;
    initReturnBuffer(&returnBuffer);

    // Flag to track if the command at the front is currently active (in DRAM processing)
    bool activeCommand = false;

    while (true) {
        // --- Display the UI (queue and command returns).
        printf("\033[2J\033[H");
        printf("\033[2J\033[H");
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
            if (strncmp(input, "VMEM", 4) == 0 || strncmp(input, "VCACHE", 6) == 0) {
                sscanf(input, "%s %hu %hd", cmd, &addr, &value);
                stage[0] = '\0';  // VMEM/VCACHE do not need a stage.
            } else {
                sscanf(input, "%s %2s %hu %hd", cmd, stage, &addr, &value);
            }
        } else {
            addCommandReturn(&returnBuffer, "Invalid input format.");
            continue;
        }

        // --- Prepare the command element.
        strcpy(element.cmd, cmd);
        strcpy(element.stage, stage);
        element.addr = addr;
        element.value = value;

        // --- Process the new command.
        if (strcmp(element.cmd, "VMEM") == 0) {
            // Process VMEM instantly.
            char memBlock[128];
            viewBlockMemory(dram, element.addr, element.value, memBlock);
            addCommandReturn(&returnBuffer, memBlock);
        } else {
            // Enqueue SW and LW commands.
            enqueue(q, &element);
            addCommandReturn(&returnBuffer, "Added to Queue.");
        }

        // --- If DRAM is idle and no active command is set, load the front command from the queue.
        if (dram->state == DRAM_IDLE && !activeCommand && !isEmpty(q)) {
            cmdElement curr = q->items[q->front]; // Peek without dequeueing
            if (strcmp(curr.cmd, "SW") == 0) {
                dram->state = DRAM_WRITE;
                dram->delayCounter = DRAM_DELAY;
                dram->pendingAddr = curr.addr;
                dram->pendingValue = curr.value;
                strcpy(dram->pendingCmd, curr.cmd);
                activeCommand = true;
            } else if (strcmp(curr.cmd, "LW") == 0) {
                dram->state = DRAM_READ;
                dram->delayCounter = DRAM_DELAY;
                dram->pendingAddr = curr.addr;
                strcpy(dram->pendingCmd, curr.cmd);
                activeCommand = true;
            }
        }

        // --- Update DRAM (simulate one cycle).
        updateDRAM(dram, &returnBuffer);

        // --- If DRAM has finished processing the active command (i.e. state is IDLE)
        // then remove the command from the queue and clear the active flag.
        if (dram->state == DRAM_IDLE && activeCommand) {
            dequeue(q);
            activeCommand = false;
        }
    }
}


int main(){
    // Initialize DRAM.
    DRAM dram;
    clearMemory(&dram);
    dram.state = DRAM_IDLE;
    dram.delayCounter = 0;
    dram.pendingAddr = 0;
    dram.pendingValue = 0;
    strcpy(dram.pendingCmd, "");

    // Initialize the command queue.
    Queue q;
    initQueue(&q);

    simulationLoop(&dram, &q);
    return 0;
}
