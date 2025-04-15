/* decode.c */
#include <stdio.h>
#include <stdlib.h>
#include "decode.h"
#include "../memory.h"
#include "decode.h"
#include "../pipeline.h"

extern DRAM dram;
extern REGISTERS* registers;

bool decode_ready(PipelineState* pipeline) {
    return pipeline->IF_ID.valid;
}

void decode_stage(PipelineState* pipeline) {
    uint16_t instruction = pipeline->IF_ID.instruction;
    uint16_t pc = pipeline->IF_ID.pc;
    char instruction_text[50];

    // If the fetched instruction is 0, propagate a NOP (bubble)
    if (instruction == 0) {
        sprintf(instruction_text, "NOP");
        pipeline->ID_EX_next.valid = false; // mark bubble
        pipeline->ID_EX_next.pc = pc;
        pipeline->ID_EX_next.opcode = 0;
        pipeline->ID_EX_next.regD = 0;
        pipeline->ID_EX_next.regA = 0;
        pipeline->ID_EX_next.regB = 0;
        pipeline->ID_EX_next.imm = 0;
    } else {
        // Otherwise, propagate a valid instruction.
        pipeline->ID_EX_next.valid = true;
        uint16_t opcode = (instruction >> 12) & 0xF;
        uint16_t rd = (instruction >> 8) & 0xF;
        uint16_t ra = (instruction >> 4) & 0xF;
        uint16_t rb_imm = instruction & 0xF;
        pipeline->ID_EX_next.pc = pc;
        pipeline->ID_EX_next.opcode = opcode;
        pipeline->ID_EX_next.regD = rd;
        pipeline->ID_EX_next.regA = ra;
        pipeline->ID_EX_next.regB = rb_imm;
        pipeline->ID_EX_next.imm = rb_imm;
        pipeline->ID_EX_next.type = rd; // e.g. used for shift type

        switch (opcode) {
            case 0: sprintf(instruction_text, "ADD R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 1: sprintf(instruction_text, "SUB R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 2: sprintf(instruction_text, "NAND R%d, R%d, R%d", rd, ra, rb_imm); break;
            case 3: sprintf(instruction_text, "LUI R%d, %d", rd, rb_imm); break;
            case 4:
            case 10: sprintf(instruction_text, "SW [R%d + %d], R%d", ra, rb_imm, rd); break;
            case 5:
            case 9: sprintf(instruction_text, "LW R%d, [R%d + %d]", rd, ra, rb_imm); break;
            case 11: sprintf(instruction_text, "BEQ R%d, R%d, %d", rd, ra, rb_imm); break;
            case 7: sprintf(instruction_text, "JALR R%d, R%d", rd, ra); break;
            default: 
                sprintf(instruction_text, "NOP or Unknown");
                pipeline->ID_EX_next.valid = false;
                break;
        }
    }

    printf("[PIPELINE]DECODE:%s:%d\n", instruction_text, pc);
    fflush(stdout);
}
