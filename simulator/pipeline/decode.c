// This file will take the response from fetch, and decode the binary encoding to get the information for execute.
#include <stdio.h>
#include <stdlib.h>
#include "fetch.h"
#include "../memory.h"

extern DRAM dram;
extern REGISTERS* registers;

bool decode_ready(PipelineState* pipeline) {
    return pipeline->IF_ID.valid;
}

void decode_stage(PipelineState* pipeline) {
    // Check if decode stage is ready
    if (!decode_ready(pipeline)) {
        return;
    }

    // If there is something to decode, mark stage as valid
    pipeline->ID_EX_next.valid = true;

    // Extract fields from instruction
    uint16_t instruction = pipeline->IF_ID.instruction;
    uint16_t opcode = (instruction >> 12) & 0xF;  // Top 4 bits
    uint16_t rd_type = (instruction >> 8) & 0xF;  // Next 4 bits
    uint16_t ra = (instruction >> 4) & 0xF;       // Next 4 bits
    uint16_t rb_imm = instruction & 0xF;          // Last 4 bits

    // Set PC in ID/EX register
    pipeline->ID_EX_next.pc = pipeline->IF_ID.pc;

    // Read actual values from registers
    uint16_t regA_value = registers->R[ra];  // Assuming R is an array of register values
    uint16_t regB_value = registers->R[rb_imm];  // Assuming R is an array of register values

    // Decode based on opcode
    if (opcode <= 0b0111) {  // RRR Type (ADD, SUB, AND, OR, XOR, DIVMOD, MUL, CMP)
        pipeline->ID_EX_next.regD = rd_type;
        pipeline->ID_EX_next.regA = regA_value;  // Pass actual value
        pipeline->ID_EX_next.regB = regB_value;  // Pass actual value
        pipeline->ID_EX_next.imm = 0;  // Not used for RRR
    }
    else if (opcode == 0b1000) {  // RR Type (LSL, LSR, ROL, ROR)
        pipeline->ID_EX_next.regD = 0;  // Not used for RR
        pipeline->ID_EX_next.regA = regA_value;  // Pass actual value
        pipeline->ID_EX_next.regB = regB_value;  // Pass actual value
        pipeline->ID_EX_next.imm = rd_type;  // Type field used as immediate
    }
    else if (opcode >= 0b1001 && opcode <= 0b1011) {  // RRI Type (LW, SW, BEQ)
        pipeline->ID_EX_next.regD = rd_type;
        pipeline->ID_EX_next.regA = regA_value;  // Pass actual value
        pipeline->ID_EX_next.regB = 0;  // Not used for RRI
        pipeline->ID_EX_next.imm = rb_imm;
    }

    // Print decode info
    printf("[DECODE] opcode=%u rd/type=%u ra=%u rb/imm=%u\n", 
           opcode, rd_type, ra, rb_imm);
    fflush(stdout);
}
