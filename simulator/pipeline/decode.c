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
    if (opcode == 0b1001 || opcode == 0b1010) {  // LW or SW
        printf("[DECODE_MEM] Memory instruction: ");
        if (opcode == 0b1001) {
            printf("LW R%u, [R%u + %u]\n", rd, ra, rb_imm);
        } else {
            printf("SW [R%u + %u], R%u\n", ra, rb_imm, rd);
        }
    }
}
