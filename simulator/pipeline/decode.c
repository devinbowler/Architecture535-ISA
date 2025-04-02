// This file will take the response from fetch, and decode the binary encoding to get the information for execute.
#include <stdio.h>
#include <stdlib.h>
#include "decode.h"
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
    // Check if the input stage has a valid instruction
    if (!pipeline->IF_ID.valid || pipeline->IF_ID.instruction == 0) {
        // If no valid instruction to decode, propagate a bubble
        pipeline->ID_EX_next.valid = true; // We're still valid, just empty (bubble)
        pipeline->ID_EX_next.opcode = 0;   // No operation
        pipeline->ID_EX_next.pc = pipeline->IF_ID.pc; // Keep PC for tracking
        
        printf("[DECODE] No valid instruction to decode (bubble)\n");
        printf("[PIPELINE]DECODE:Bubble:%d\n", pipeline->IF_ID.pc);
        return;
    }

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
    printf("[DECODE_DEBUG] Instruction=%u PC=%u, will be passed to ID_EX_next\n", 
           instruction, pc);

    // Setup pipeline for next stage
    pipeline->ID_EX_next.pc = pc;
    pipeline->ID_EX_next.opcode = opcode;
    pipeline->ID_EX_next.regD = rd;
    pipeline->ID_EX_next.regA = ra;
    pipeline->ID_EX_next.regB = rb_imm;
    pipeline->ID_EX_next.imm = rb_imm;  // For RRI-Type instructions, rb is actually the immediate
    pipeline->ID_EX_next.type = rd;     // For RR-Type, rd is actually the type.
    
    printf("[DECODE_DEBUG] ID_EX_next now has opcode=%u, will be used for execute in next cycle\n", 
           pipeline->ID_EX_next.opcode);

    // Make sure the immediate value is used correctly for LW/SW
    if (opcode == 0b1001 || opcode == 0b1010) {  // LW or SW
        printf("[DECODE_MEM] Memory instruction: ");
        char instruction_text[50];
        if (opcode == 0b1001) {
            sprintf(instruction_text, "LW R%d, [R%d + %d]", rd, ra, rb_imm);
            printf("LW R%u, [R%u + %u]\n", rd, ra, rb_imm);
        } else {
            sprintf(instruction_text, "SW [R%d + %d], R%d", ra, rb_imm, rd);
            printf("SW [R%u + %u], R%u\n", ra, rb_imm, rd);
        }
        printf("[PIPELINE]DECODE:%s:%d\n", instruction_text, pc);
    } else {
        // Generate text for other instruction types
        char instruction_text[50] = "Unknown";
        switch(opcode) {
            case 0b0000: sprintf(instruction_text, "ADD R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 0b0001: sprintf(instruction_text, "SUB R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 0b0010: sprintf(instruction_text, "AND R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 0b0011: sprintf(instruction_text, "OR R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 0b0100: sprintf(instruction_text, "XOR R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 0b0101: sprintf(instruction_text, "DIVMOD R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 0b0110: sprintf(instruction_text, "MUL R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 0b0111: sprintf(instruction_text, "CMP R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 0b1000: sprintf(instruction_text, "SHIFT Type=%d, R%d, R%d", rd, ra, rb_imm); break;
            case 0b1011: sprintf(instruction_text, "BEQ R%d, R%d, %d", rd, ra, rb_imm); break;
            default: 
                sprintf(instruction_text, "NOP or Unknown opcode=%d", opcode);
                // Invalid opcode, should be treated as bubble
                pipeline->ID_EX_next.opcode = 0;
        }
        printf("[PIPELINE]DECODE:%s:%d\n", instruction_text, pc);
    }
}
