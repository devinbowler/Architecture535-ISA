// This file will look at the PC and do a memory access to get the next instruction.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "fetch.h"
#include "../memory.h"

extern DRAM dram;
extern REGISTERS* registers;
extern bool branch_taken;
extern uint16_t branch_target_address;

bool fetch_ready(PipelineState* pipeline) {
    return pipeline->IF_ID.valid;  // Ready to fetch when IF_ID is empty
}

/**
 * @brief Implements the fetch stage of the pipeline.
 *
 * @param pipeline the pipeline
 */
void fetch_stage(PipelineState* pipeline, uint16_t* value) {
    // Check if fetch stage is ready
    if(!pipeline->IF_ID.valid) return;

    uint16_t pc = registers->R[15];
    
    // If a branch was taken in the execute stage, update the PC
    if (branch_taken) {
        pc = branch_target_address;
        registers->R[15] = pc; // Update PC register
        branch_taken = false;  // Reset the flag
        printf("[FETCH] Branch taken, PC updated to %u\n", pc);
    }
    
    uint16_t instruction = readFromMemory(&dram, pc);

    // Extract instruction details for UI
    uint16_t opcode = (instruction >> 12) & 0xF;
    
    // Format the instruction for the UI
    char instruction_text[50] = "NOP";
    if (instruction != 0) {
        switch(opcode) {
            case 0: sprintf(instruction_text, "ADD R%d, R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 1: sprintf(instruction_text, "ADDI R%d, R%d, %d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 2: sprintf(instruction_text, "NAND R%d, R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 3: sprintf(instruction_text, "LUI R%d, %d", 
                (instruction >> 8) & 0xF, instruction & 0xF); break;
            case 4: sprintf(instruction_text, "SW R%d, R%d, %d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 5: sprintf(instruction_text, "LW R%d, R%d, %d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 11: sprintf(instruction_text, "BEQ R%d, R%d, %d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 7: sprintf(instruction_text, "JALR R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF); break;
            default: sprintf(instruction_text, "Unknown 0x%04X", instruction);
        }
    }

    // Generate and save pipeline instruction information
    *value = instruction;
    
    // Update pipeline register
    pipeline->IF_ID_next.valid = true;
    pipeline->IF_ID_next.pc = pc;
    pipeline->IF_ID_next.instruction = instruction;

    // Print fetch info
    printf("[FETCH] instruction=%u pc=%u\n", instruction, pc);
    
    // Send detailed instruction text for pipeline visualization
    if (instruction != 0) {
        printf("[PIPELINE]FETCH:%s:%d\n", instruction_text, pc);
    } else {
        printf("[PIPELINE]FETCH:Bubble:%d\n", pc);
    }
    fflush(stdout);
}