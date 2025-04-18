// pipeline/decode.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "decode.h"
#include "../memory.h"
#include "../pipeline.h"

extern DRAM        dram;
extern REGISTERS  *registers;

/**
 * Decode stage: turn IF_ID into ID_EX_next.
 */
void decode_stage(PipelineState* p) {
    // bubble?
    if (!p->IF_ID.valid) {
        p->ID_EX_next.valid = false;
        printf("[PIPELINE]DECODE:NOP:%d\n", p->IF_ID.pc);
        return;
    }

    uint16_t instr = p->IF_ID.instruction;
    uint16_t pc    = p->IF_ID.pc;
    char instruction_text[64];

    if (instr == 0) {
        // explicit NOP
        p->ID_EX_next.valid  = false;
        sprintf(instruction_text, "NOP");
    } else {
        // field extraction
        uint16_t op   = (instr >> 12) & 0xF;
        uint16_t rd   = (instr >>  8) & 0xF;
        uint16_t ra   = (instr >>  4) & 0xF;
        uint16_t imm  =  instr        & 0xF;
        uint16_t type = 0;

        // shifts (opcode 8) use rd as TYPE and imm as destination in R-type
        if (op == 0x8) {
            type = rd;
            rd   = imm;
            imm  = 1;
            ra   = (instr >> 4) & 0xF;
        }

        // populate ID/EX next
        p->ID_EX_next.valid  = true;
        p->ID_EX_next.pc     = pc;
        p->ID_EX_next.opcode = op;
        p->ID_EX_next.regD   = rd;
        p->ID_EX_next.regA   = ra;
        p->ID_EX_next.regB   = imm;
        p->ID_EX_next.imm    = imm;
        p->ID_EX_next.type   = type;

        // human‐readable for UI
        switch (op) {
            case 0x0: sprintf(instruction_text, "ADD    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x1: sprintf(instruction_text, "SUB    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x2: sprintf(instruction_text, "AND    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x3: sprintf(instruction_text, "OR     R%u, R%u, R%u", rd, ra, imm); break;
            case 0x4: sprintf(instruction_text, "XOR    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x5: sprintf(instruction_text, "DIVMOD R%u, R%u, R%u", rd, ra, imm); break;
            case 0x6: sprintf(instruction_text, "MUL    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x7: sprintf(instruction_text, "CMP    R%u, R%u, R%u", rd, ra, imm); break;
            case 0x8: {
                const char* opn = (type==0?"LSL": type==1?"LSR":
                                  type==2?"ROL":"ROR");
                sprintf(instruction_text, "%s    R%u, R%u, %u", opn, rd, ra, imm);
                break;
            }
            case 0x9: sprintf(instruction_text, "LW     R%u, [R%u + %u]", rd, ra, imm); break;
            case 0xA: sprintf(instruction_text, "SW     [R%u + %u], R%u", ra, imm, rd); break;
            case 0xB: sprintf(instruction_text, "BEQ    R%u, R%u, %u", rd, ra, imm); break;
            case 0xF: sprintf(instruction_text, "BLT    R%u, R%u, %u", rd, ra, imm); break;
            default:
                sprintf(instruction_text, "UNKNOWN");
                p->ID_EX_next.valid = false;
        }
    }
    printf("[PIPELINE]DECODE:%s:%d\n", instruction_text, pc);
    fflush(stdout);
}