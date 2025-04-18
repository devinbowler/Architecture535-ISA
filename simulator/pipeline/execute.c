// pipeline/execute.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "execute.h"
#include "../pipeline.h"
#include "../memory.h"
#include "../globals.h"    // for any global offsets

extern REGISTERS *registers;
bool branch_taken = false;
uint16_t branch_target_address = 0;

/**
 * Flush the front of the pipeline on a taken branch.
 */
void flush_pipeline(PipelineState *p) {
    p->IF_ID.valid      = false;
    p->ID_EX.valid      = false;
    p->IF_ID_next.valid = false;
    p->ID_EX_next.valid = false;
    printf("[PIPELINE] Branch detected: Flushing pipeline\n");
}

/**
 * Execute stage: perform ALU / branch and push into EX_MEM_next.
 */
void execute(PipelineState *p) {
    uint16_t pc = p->ID_EX.pc;

    if (!p->ID_EX.valid) {
        p->EX_MEM_next.valid = false;
        printf("[PIPELINE]EXECUTE:NOP:%d\n", pc);
        fflush(stdout);
        return;
    }

    // propagate common fields
    p->EX_MEM_next.valid   = true;
    p->EX_MEM_next.pc      = pc;
    p->EX_MEM_next.opcode  = p->ID_EX.opcode;
    p->EX_MEM_next.regD    = p->ID_EX.regD;
    p->EX_MEM_next.regA    = p->ID_EX.regA;
    p->EX_MEM_next.regB    = p->ID_EX.regB;
    p->EX_MEM_next.imm     = p->ID_EX.imm;
    p->EX_MEM_next.resMod  = 0;

    uint16_t op   = p->ID_EX.opcode;
    uint16_t d    = p->ID_EX.regD;
    uint16_t a    = p->ID_EX.regA;
    uint16_t bimm = p->ID_EX.imm;
    uint16_t res  = 0;
    uint16_t vA   = registers->R[a];
    uint16_t vB   = registers->R[p->ID_EX.regB];
    char txt[64];

    switch (op) {
        case 0x0:  // ADD
            res = vA + vB;
            sprintf(txt, "ADD R%u,R%u,R%u", d, a, p->ID_EX.regB);
            printf("[EXECUTE_ADD] R%u = %u + %u = %u\n", d, vA, vB, res);
            break;
        case 0x1:  // SUB
            res = vA - vB;
            sprintf(txt, "SUB R%u,R%u,R%u", d, a, p->ID_EX.regB);
            printf("[EXECUTE_SUB] R%u = %u - %u = %u\n", d, vA, vB, res);
            break;
        case 0x2:  // NAND
            res = ~(vA & vB);
            sprintf(txt, "NAND R%u,R%u,R%u", d, a, p->ID_EX.regB);
            printf("[EXECUTE_NAND] R%u = ~( %u & %u ) = %u\n", d, vA, vB, res);
            break;
        case 0x3:  // LUI
            res = (vB << 8) & 0xFF00;
            sprintf(txt, "LUI R%u,%u", d, p->ID_EX.regB);
            printf("[EXECUTE_LUI] R%u = %u << 8 = %u\n", d, p->ID_EX.regB, res);
            break;
        case 0x5:  // DIVMOD
            if (vB == 0) {
                res = 0; p->EX_MEM_next.resMod = 0;
                sprintf(txt, "DIVMOD R%u,R%u,R%u (Div0)", d, a, p->ID_EX.regB);
                printf("[EXECUTE_DIVMOD] Div by zero → result=0, rem=0\n");
            } else {
                res = vA / vB;
                p->EX_MEM_next.resMod = vA % vB;
                sprintf(txt, "DIVMOD R%u,R%u,R%u", d, a, p->ID_EX.regB);
                printf("[EXECUTE_DIVMOD] R%u = %u / %u = %u, rem = %u\n",
                       d, vA, vB, res, p->EX_MEM_next.resMod);
            }
            break;
        case 0x6:  // MUL
            res = vA * vB;
            sprintf(txt, "MUL R%u,R%u,R%u", d, a, p->ID_EX.regB);
            printf("[EXECUTE_MUL] R%u = %u * %u = %u\n", d, vA, vB, res);
            break;
        case 0x7:  // CMP
            res = (int16_t)registers->R[d] - (int16_t)vA;
            registers->R[14] = res;
            sprintf(txt, "CMP R%u,R%u,R%u", d, a, p->ID_EX.regB);
            printf("[EXECUTE_CMP] SR = R%u - %u = %u\n", d, vA, res);
            break;
        case 0x8: { // shifts/rotates
            uint16_t t = p->ID_EX.type, i = bimm;
            uint16_t opnd = registers->R[a];
            if      (t==0) res = opnd << i;
            else if (t==1) res = opnd >> i;
            else if (t==2) res = (opnd << i) | (opnd >> (16 - i));
            else           res = (opnd >> i) | (opnd << (16 - i));
            sprintf(txt, "SH  t=%u R%u,R%u,%u", t, d, a, i);
            break;
        }
        case 0x9:  // LW
            res = registers->R[a] + bimm;
            sprintf(txt, "LW  R%u,[R%u+%u] @%u", d, a, bimm, res);
            printf("[EXECUTE_LW] addr = %u + %u = %u\n", registers->R[a], bimm, res);
            break;
        case 0xA:  // SW
            res = registers->R[a] + bimm;
            sprintf(txt, "SW  [R%u+%u],R%u @%u", a, bimm, d, res);
            printf("[EXECUTE_SW] addr = %u + %u = %u\n", registers->R[a], bimm, res);
            break;
        case 0xB:  // BEQ
            if (registers->R[d] == registers->R[a]) {
                branch_taken = true;
                branch_target_address = pc + (bimm + 1);
                flush_pipeline(p);
                printf("[EXECUTE_BEQ] Branch taken → PC=%u\n", branch_target_address);
            } else {
                printf("[EXECUTE_BEQ] Not taken\n");
            }
            sprintf(txt, "BEQ R%u,R%u,%u", d, a, (bimm + 1));
            break;
        case 0xF:  // BLT
            if ((int16_t)registers->R[d] < (int16_t)registers->R[a]) {
                branch_taken = true;
                branch_target_address = pc + bimm;
                flush_pipeline(p);
                printf("[EXECUTE_BLT] Branch taken → PC=%u\n", branch_target_address);
            } else {
                printf("[EXECUTE_BLT] Not taken\n");
            }
            sprintf(txt, "BLT R%u,R%u,%u", d, a, bimm);
            break;
        default:
            sprintf(txt, "UNK");
            p->EX_MEM_next.valid = false;
            break;
    }

    // shove result into next latch
    p->EX_MEM_next.res = res;
    printf("[PIPELINE]EXECUTE:%s:%d\n", txt, pc);
    fflush(stdout);
}