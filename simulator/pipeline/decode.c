// pipeline/decode.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "decode.h"
#include "../memory.h"
#include "../pipeline.h"

extern DRAM        dram;
extern REGISTERS  *registers;

void decode_stage(PipelineState *p)
{
    if (!p->IF_ID.valid) {
        p->ID_EX_next.valid = false;
        printf("[PIPELINE]DECODE:NOP:%d\n", p->IF_ID.pc);
        return;
    }

    uint16_t ins = p->IF_ID.instruction;
    uint16_t pc  = p->IF_ID.pc;
    char     txt[64];

    if (ins == 0) {
        p->ID_EX_next.valid = false;
        sprintf(txt, "NOP");
    } else {
        uint16_t op = ins >> 12;

        if (op == 0x8) {
            // single‐opcode shifts/rotates
            // bits[11:8]=type, bits[7:4]=Rd (also source), bits[3:0]=Rs (amt‐reg)
            uint16_t type = (ins >> 8) & 0xF;
            uint16_t rd   = (ins >> 4) & 0xF;
            uint16_t rs   =  ins        & 0xF;

            p->ID_EX_next.valid   = true;
            p->ID_EX_next.pc      = pc;
            p->ID_EX_next.opcode  = op;
            p->ID_EX_next.type    = type;
            p->ID_EX_next.regD    = rd;   // destination
            p->ID_EX_next.regA    = rd;   // source value
            p->ID_EX_next.regB    = rs;   // which register has shift amount
            p->ID_EX_next.imm     = 0;    // unused here

            const char *name = (type == 0) ? "LSL"
                              : (type == 1) ? "LSR"
                              : (type == 2) ? "ROL"
                                            : "ROR";
            sprintf(txt, "%s R%u, R%u, R%u", name, rd, rd, rs);
        }
        else {
            // “normal” RRR/RRI decoding
            uint16_t rd  = (ins >>  8) & 0xF;
            uint16_t ra  = (ins >>  4) & 0xF;
            uint16_t imm =  ins        & 0xF;

            p->ID_EX_next.valid   = true;
            p->ID_EX_next.pc      = pc;
            p->ID_EX_next.opcode  = op;
            p->ID_EX_next.regD    = rd;
            p->ID_EX_next.regA    = ra;
            p->ID_EX_next.regB    = imm;
            p->ID_EX_next.imm     = imm;
            p->ID_EX_next.type    = 0;

            switch (op) {
                case 0x0: sprintf(txt, "ADD    R%u,R%u,%u", rd, ra, imm); break;
                case 0x1: sprintf(txt, "SUB    R%u,R%u,%u", rd, ra, imm); break;
                case 0x2: sprintf(txt, "AND    R%u,R%u,%u", rd, ra, imm); break;
                case 0x3: sprintf(txt, "OR     R%u,R%u,%u", rd, ra, imm); break;
                case 0x4: sprintf(txt, "XOR    R%u,R%u,%u", rd, ra, imm); break;
                case 0x5: sprintf(txt, "DIVMOD R%u,R%u,%u", rd, ra, imm); break;
                case 0x6: sprintf(txt, "MUL    R%u,R%u,%u", rd, ra, imm); break;
                case 0x7: sprintf(txt, "CMP    R%u,R%u,%u", rd, ra, imm); break;
                case 0x9: sprintf(txt, "LW     R%u,[R%u+%u]",   rd, ra, imm); break;
                case 0xA: sprintf(txt, "SW     [R%u+%u],R%u",   ra, imm, rd); break;
                case 0xB: sprintf(txt, "BEQ    R%u,R%u,%u",     rd, ra, imm); break;
                case 0xF: sprintf(txt, "BLT    R%u,R%u,%u",     rd, ra, imm); break;
                default:  sprintf(txt, "UNKNOWN"); p->ID_EX_next.valid = false;
            }
        }
    }
    printf("[PIPELINE]DECODE:%s:%d\n", instruction_text, pc);
    fflush(stdout);
}
