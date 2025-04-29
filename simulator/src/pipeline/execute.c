// pipeline/execute.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "execute.h"
#include "pipeline.h"
#include "memory.h"
#include "globals.h"    // for DATA_OFFSET, delays, etc.

extern REGISTERS *registers;
bool branch_taken = false;
uint16_t branch_target_address = 0;

void execute(PipelineState *p) {
    uint16_t pc = p->ID_EX.pc;

    if (!p->ID_EX.valid) {
        p->EX_MEM_next.valid = false;
        printf("[PIPELINE]EXECUTE:NOP:%d\n", pc);
        fflush(stdout);
        return;
    }

    // If this instruction is squashed, just propagate it with the squashed flag
    if (p->ID_EX.squashed) {
        p->EX_MEM_next.valid = true;
        p->EX_MEM_next.squashed = true;
        p->EX_MEM_next.pc = pc;
        p->EX_MEM_next.opcode = p->ID_EX.opcode;
        p->EX_MEM_next.regD = p->ID_EX.regD;
        printf("[PIPELINE]EXECUTE:SQUASHED:%d\n", pc);
        fflush(stdout);
        return;
    }

    // common propagation for non-squashed instructions
    p->EX_MEM_next.valid = true;
    p->EX_MEM_next.squashed = false;  // Explicitly mark as not squashed
    p->EX_MEM_next.pc = pc;
    p->EX_MEM_next.opcode = p->ID_EX.opcode;
    p->EX_MEM_next.regD = p->ID_EX.regD;
    p->EX_MEM_next.regA = p->ID_EX.regA;
    p->EX_MEM_next.regB = p->ID_EX.regB;
    p->EX_MEM_next.imm = p->ID_EX.imm;
    p->EX_MEM_next.resMod = 0;

    uint16_t op = p->ID_EX.opcode;
    uint16_t d = p->ID_EX.regD;
    uint16_t a = p->ID_EX.regA;
    uint16_t rb = p->ID_EX.regB;
    uint16_t imm = p->ID_EX.imm;
    uint16_t res = 0;
    uint16_t vA = registers->R[a];
    uint16_t vB = registers->R[rb];
    char txt[64];

    switch (op) {
        case 0x0:  // ADD
            res = vA + vB;
            sprintf(txt, "ADD R%u,R%u,R%u", d, a, rb);
            printf("[EXECUTE_ADD] R%u = %u + %u = %u\n", d, vA, vB, res);
            break;
        case 0x1:  // SUB
            res = vA - vB;
            sprintf(txt, "SUB R%u,R%u,R%u", d, a, rb);
            printf("[EXECUTE_SUB] R%u = %u - %u = %u\n", d, vA, vB, res);
            break;
        case 0x2:  // AND
            res = vA & vB;
            sprintf(txt, "AND R%u,R%u,R%u", d, a, rb);
            printf("[EXECUTE_AND] R%u = %u & %u = %u\n", d, vA, vB, res);
            break;
        case 0x3:  // OR
            res = vA | vB;
            sprintf(txt, "OR  R%u,R%u,R%u", d, a, rb);
            printf("[EXECUTE_OR] R%u = %u | %u = %u\n", d, vA, vB, res);
            break;
        case 0x4:  // XOR
            res = vA ^ vB;
            sprintf(txt, "XOR R%u,R%u,R%u", d, a, rb);
            printf("[EXECUTE_XOR] R%u = %u ^ %u = %u\n", d, vA, vB, res);
            break;
        case 0x5:  // DIVMOD
            if (vB == 0) {
                res = 0; p->EX_MEM_next.resMod = 0;
                sprintf(txt, "DIVMOD R%u,R%u,R%u (div0)", d, a, rb);
                printf("[EXECUTE_DIVMOD] Divide by zero → 0\n");
            } else {
                res = vA / vB;
                p->EX_MEM_next.resMod = vA % vB;
                sprintf(txt, "DIVMOD R%u,R%u,R%u", d, a, rb);
                printf("[EXECUTE_DIVMOD] R%u = %u / %u = %u rem %u\n",
                       d, vA, vB, res, p->EX_MEM_next.resMod);
            }
            break;
        case 0x6:  // MUL
            res = vA * vB;
            sprintf(txt, "MUL R%u,R%u,R%u", d, a, rb);
            printf("[EXECUTE_MUL] R%u = %u * %u = %u\n", d, vA, vB, res);
            break;
        case 0x7:  // CMP
            res = (int16_t)registers->R[d] - (int16_t)vA;
            registers->R[14] = res;
            sprintf(txt, "CMP R%u,R%u,R%u", d, a, rb);
            printf("[EXECUTE_CMP] SR = R%u - %u = %u\n", d, vA, res);
            break;
        case 0x8: {  // shifts/rotates
            uint16_t t = p->ID_EX.type;  // 0=LSL,1=LSR,2=ROL,3=ROR
            uint16_t rd = d;               // original dest
            uint16_t rs = rb;              // amt‐reg
            uint16_t opnd = registers->R[rd];
            uint16_t amount = registers->R[rs];
            const char *name = (t == 0 ? "LSL" : t == 1 ? "LSR" : t == 2 ? "ROL" : "ROR");

            if (t == 0) res = opnd << amount;
            else if (t == 1) res = opnd >> amount;
            else if (t == 2) res = (opnd << amount) | (opnd >> (16 - amount));
            else res = (opnd >> amount) | (opnd << (16 - amount));

            p->EX_MEM_next.res = res;
            sprintf(txt, "%s R%u, R%u, %u", name, rd, rd, amount);
            printf("[EXECUTE_%s] R%u = R%u %s %u → %u\n",
                   name, rd, rd, (t < 2 ? "<<" : ">>"), amount, res);
            break;
        }
        case 0x9:  // LW
            res = registers->R[a] + imm;
            sprintf(txt, "LW  R%u,[R%u+%u]", d, a, imm);
            printf("[EXECUTE_LW] addr = %u + %u = %u\n", registers->R[a], imm, res);
            break;
        case 0xA:  // SW
            res = registers->R[a] + imm;
            sprintf(txt, "SW  [R%u+%u],R%u", a, imm, d);
            printf("[EXECUTE_SW] addr = %u + %u = %u\n", registers->R[a], imm, res);
            break;
        case 0xB:  // BEQ
            if (registers->R[d] == registers->R[a]) {
                // Branch will be taken, but PC will be updated at writeback
                branch_taken = true;
                branch_target_address = pc + imm + 1;
                
                // Mark subsequent instructions as squashed
                mark_subsequent_instructions_as_squashed(p);
                
                printf("[EXECUTE_BEQ] Branch will be taken → PC=%u (will update at writeback)\n", 
                       branch_target_address);
            } else {
                printf("[EXECUTE_BEQ] not taken\n");
            }
            sprintf(txt, "BEQ R%u,R%u,%u", d, a, imm + 1);
            break;
        case 0xF:  // BLT
            if ((int16_t)registers->R[d] < (int16_t)registers->R[a]) {
                // Branch will be taken, but PC will be updated at writeback
                branch_taken = true;
                branch_target_address = pc + imm;
                
                // Mark subsequent instructions as squashed
                mark_subsequent_instructions_as_squashed(p);
                
                printf("[EXECUTE_BLT] Branch will be taken → PC=%u (will update at writeback)\n", 
                       branch_target_address);
            } else {
                printf("[EXECUTE_BLT] not taken\n");
            }
            sprintf(txt, "BLT R%u,R%u,%u", d, a, imm);
            break;
        default:
            sprintf(txt, "UNKNOWN");
            p->EX_MEM_next.valid = false;
            break;
    }

    // write‐out and trace
    p->EX_MEM_next.res = res;
    printf("[PIPELINE]EXECUTE:%s:%d\n", txt, pc);
    fflush(stdout);
}
