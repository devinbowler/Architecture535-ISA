#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "decode.h"
#include "../memory.h"
#include "../pipeline.h"

extern DRAM dram;
extern REGISTERS* registers;

void decode_stage(PipelineState* pipeline) {
    if (!pipeline->IF_ID.valid) {
        pipeline->IF_ID_next.valid = false;
        printf("[PIPELINE]DECODE:NOP:%d\n", pipeline->IF_ID.pc);
        return;
    }

    uint16_t instr = pipeline->IF_ID.instruction;
    uint16_t pc    = pipeline->IF_ID.pc;
    char instruction_text[64];

    if (instr == 0) {
        // NOP
        sprintf(instruction_text, "NOP");
        pipeline->ID_EX_next.valid  = false;
        pipeline->ID_EX_next.pc     = pc;
        pipeline->ID_EX_next.opcode = 0;
        pipeline->ID_EX_next.regD   = 0;
        pipeline->ID_EX_next.regA   = 0;
        pipeline->ID_EX_next.regB   = 0;
        pipeline->ID_EX_next.imm    = 0;
    } else {
        pipeline->ID_EX_next.valid = true;
        uint16_t opcode = (instr >> 12) & 0xF;
        uint16_t rd     = (instr >> 8)  & 0xF;
        uint16_t ra     = (instr >> 4)  & 0xF;
        uint16_t imm    = instr & 0xF;

        pipeline->ID_EX_next.pc     = pc;
        pipeline->ID_EX_next.opcode = opcode;
        pipeline->ID_EX_next.regD   = rd;
        pipeline->ID_EX_next.regA   = ra;
        pipeline->ID_EX_next.regB   = imm;
        pipeline->ID_EX_next.imm    = imm;
        pipeline->ID_EX_next.type   = rd;  // for shifts

        switch (opcode) {
            case 0x0: sprintf(instruction_text, "ADD    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x1: sprintf(instruction_text, "SUB    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x2: sprintf(instruction_text, "AND    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x3: sprintf(instruction_text, "OR     R%u, R%u, R%u", rd, ra, imm); break;
            case 0x4: sprintf(instruction_text, "XOR    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x5: sprintf(instruction_text, "DIVMOD R%u, R%u, R%u", rd, ra, imm); break;
            case 0x6: sprintf(instruction_text, "MUL    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x7: sprintf(instruction_text, "CMP    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x8: {
                uint16_t st = pipeline->ID_EX_next.type;
                const char* op = (st==0?"LSL":st==1?"LSR":st==2?"ROL":"ROR");
                sprintf(instruction_text, "%s    R%u, R%u, %u", op, rd, ra, imm);
                break;
            }
            case 0x9: sprintf(instruction_text, "LW     R%u, [R%u + %u]", rd, ra, imm); break;
            case 0xA: sprintf(instruction_text, "SW     [R%u + %u], R%u", ra, imm, rd); break;
            case 0xB: sprintf(instruction_text, "BEQ    R%u, R%u, %u", rd, ra, imm); break;
            case 0xF: sprintf(instruction_text, "BLT    R%u, R%u, %u", rd, ra, imm); break;
            default:
                sprintf(instruction_text, "UNKNOWN");
                pipeline->ID_EX_next.valid = false;
                break;
        }
    }
    printf("[PIPELINE]DECODE:%s:%d\n", instruction_text, pc);
    fflush(stdout);
}