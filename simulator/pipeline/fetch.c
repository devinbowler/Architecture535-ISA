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

void fetch_stage(PipelineState* pipeline, uint16_t* value) {
    // Mark the next register as valid.
    pipeline->IF_ID_next.valid = true;
    pipeline->IF_ID_next.instruction = *value;


    // If a branch was taken in the execute stage, update the PC
    if (branch_taken) {
        registers->R[15] = branch_target_address; // Update PC register
        branch_taken = false;  // Reset the flag
        printf("[FETCH] Branch taken, PC updated to %u\n", registers->R[15]);
    }

    uint16_t pc = registers->R[15];
    uint16_t instruction = readFromMemory(&dram, pc);

    // Extract instruction details for UI
    uint16_t opcode = (instruction >> 12) & 0xF;
    
    // Format the instruction for the UI
    char instruction_text[50];
    if (instruction == 0) {
        sprintf(instruction_text, "NOP");
    } else {
    char instruction_text[50];
    if (instruction == 0) {
        sprintf(instruction_text, "NOP");
    } else {
        uint16_t rd = (instruction >> 8) & 0xF;
        uint16_t ra = (instruction >> 4) & 0xF;
        uint16_t rb_imm = instruction & 0xF;
        
        switch(opcode) {
            case 0: sprintf(instruction_text, "ADD R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 1: sprintf(instruction_text, "SUB R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 2: sprintf(instruction_text, "NAND R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 3: sprintf(instruction_text, "LUI R%d, %d", rd, rb_imm); break;
            case 4: // Old SW opcode
            case 10: // New SW opcode
                sprintf(instruction_text, "SW [R%d + %d], R%d", ra, rb_imm, rd); break;
            case 5: // Old LW opcode
            case 9: // New LW opcode
                sprintf(instruction_text, "LW R%d, [R%d + %d]", rd, ra, rb_imm); break;
            case 11: sprintf(instruction_text, "BEQ R%d, R%d, %d", rd, ra, rb_imm); break;
            case 7: sprintf(instruction_text, "JALR R%d, R%d", rd, ra); break;
            default: sprintf(instruction_text, "Unknown 0x%04X", instruction);
        }
    }
    
    // Update pipeline register
    pipeline->IF_ID_next.pc = pc;
    pipeline->IF_ID_next.instruction = instruction;

    // Generate and save pipeline instruction information
    *value = instruction;

    // Print fetch info
    printf("[FETCH] instruction=%u pc=%u\n", instruction, pc);
    
    // Send detailed instruction text for pipeline visualization
    printf("[PIPELINE]FETCH:%s:%d\n", instruction_text, pc);
    
    fflush(stdout);
}