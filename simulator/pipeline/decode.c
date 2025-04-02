// This file will take the response from fetch, and decode the binary encoding to get the information for execute.
#include <stdio.h>
#include <stdlib.h>
#include "fetch.h"
#include "../memory.h"
#include "decode.h"
#include "pipeline.h"

extern DRAM dram;
extern REGISTERS* registers;

bool decode_ready(PipelineState* pipeline) {
    return pipeline->IF_ID.valid;
}

/**
 * @brief Implements the decode stage of the pipeline.
 * @param pipeline the pipeline
 */
void decode_stage(PipelineState* pipeline) {
    // Check if decode stage is ready
    //if (!decode_ready(pipeline)) {
    //    return;
    //}

    // If there is something to decode, mark stage as valid
    pipeline->ID_EX_next.valid = true;

    uint16_t instruction = pipeline->IF_ID.instruction;
    uint16_t pc = pipeline->IF_ID.pc;

    // Extract fields from instruction
    uint16_t opcode = (instruction >> 12) & 0xF;
    uint16_t rd = (instruction >> 8) & 0xF;
    uint16_t ra = (instruction >> 4) & 0xF;
    uint16_t rb_imm = instruction & 0xF;

    // Debug all fields to ensure correct values
    printf("[DECODE] opcode=%u rd=%u ra=%u rb=%u\n", opcode, rd, ra, rb_imm);

    // Setup pipeline for next stage
    pipeline->ID_EX_next.pc = pc;
    pipeline->ID_EX_next.opcode = opcode;
    pipeline->ID_EX_next.regD = rd;
    pipeline->ID_EX_next.regA = ra;
    pipeline->ID_EX_next.regB = rb_imm;
    pipeline->ID_EX_next.imm = rb_imm;  // For RRI-Type instructions, rb is actually the immediate
    pipeline->ID_EX_next.type = rd;     // For RR-Type, rd is actually the type.

    // Make sure the immediate value is used correctly for LW/SW
    if (opcode == 5 || opcode == 4 || opcode == 9 || opcode == 10) {  // LW or SW (both old and new opcodes)
        printf("[DECODE_MEM] Memory instruction: ");
        char instruction_text[50];
        if (opcode == 5 || opcode == 9) {  // LW (both old and new opcode)
            sprintf(instruction_text, "LW R%d, [R%d + %d]", rd, ra, rb_imm);
            printf("LW R%u, [R%u + %u]\n", rd, ra, rb_imm);
        } else {  // SW (both old and new opcode)
            sprintf(instruction_text, "SW [R%d + %d], R%d", ra, rb_imm, rd);
            printf("SW [R%u + %u], R%u\n", ra, rb_imm, rd);
        }
        printf("[PIPELINE]DECODE:%s:%d\n", instruction_text, pc);
    } else {
        // Generate text for other instruction types
        char instruction_text[50] = "Unknown";
        switch(opcode) {
            case 0: sprintf(instruction_text, "ADD R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 1: sprintf(instruction_text, "SUB R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 2: sprintf(instruction_text, "NAND R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 3: sprintf(instruction_text, "LUI R%d, %d", rd, rb_imm); break;
            case 7: sprintf(instruction_text, "JALR R%d, R%d", rd, ra); break;
            case 9: sprintf(instruction_text, "LW R%d, [R%d + %d]", rd, ra, rb_imm); break;
            case 10: sprintf(instruction_text, "SW [R%d + %d], R%d", ra, rb_imm, rd); break;
            case 11: sprintf(instruction_text, "BEQ R%d, R%d, %d", rd, ra, rb_imm); break;
            default: sprintf(instruction_text, "NOP or Unknown (%d)", opcode);
        }
        printf("[PIPELINE]DECODE:%s:%d\n", instruction_text, pc);
    }
    
    // Always show instruction and PC, even during bubbles
    fflush(stdout);
}
