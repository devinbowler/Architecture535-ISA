// This file will look at the PC and do a memory access to get the next instruction.
#include <stdio.h>
#include <stdlib.h>
#include "fetch.h"
#include "../memory.h"

extern DRAM dram;
extern REGISTERS* registers;

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
    uint16_t instruction = readFromMemory(&dram, pc);

    // Extract instruction details for UI
    uint16_t opcode = (instruction >> 12) & 0xF;
    
    // Format the instruction for the UI
    char instruction_text[50] = "NOP";
    if (instruction != 0) {
        switch(opcode) {
            case 0b0000: sprintf(instruction_text, "ADD R%d, R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 0b0001: sprintf(instruction_text, "SUB R%d, R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 0b0010: sprintf(instruction_text, "AND R%d, R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 0b0011: sprintf(instruction_text, "OR R%d, R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 0b0100: sprintf(instruction_text, "XOR R%d, R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 0b0101: sprintf(instruction_text, "DIVMOD R%d, R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 0b0110: sprintf(instruction_text, "MUL R%d, R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 0b0111: sprintf(instruction_text, "CMP R%d, R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 0b1000: sprintf(instruction_text, "SHIFT R%d, R%d, R%d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 0b1001: sprintf(instruction_text, "LW R%d, [R%d + %d]", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
            case 0b1010: sprintf(instruction_text, "SW [R%d + %d], R%d", 
                (instruction >> 4) & 0xF, instruction & 0xF, (instruction >> 8) & 0xF); break;
            case 0b1011: sprintf(instruction_text, "BEQ R%d, R%d, %d", 
                (instruction >> 8) & 0xF, (instruction >> 4) & 0xF, instruction & 0xF); break;
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