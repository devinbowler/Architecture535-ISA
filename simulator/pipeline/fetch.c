// fetch.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "fetch.h"
#include "../memory.h"
#include "../pipeline.h"

extern DRAM        dram;
extern REGISTERS  *registers;
extern bool        branch_taken;
extern uint16_t    branch_target_address;

/**
 * Decode a raw 16‑bit instruction into a display string.
 */
static void fmt_instr(uint16_t instr, char *out) {
    if (instr == 0) {
        sprintf(out, "NOP");
        return;
    }

    uint16_t op    = (instr >> 12) & 0xF;
    uint16_t rd    = (instr >>  8) & 0xF;  // For RRR/RRI: dest. For RR: type
    uint16_t ra    = (instr >>  4) & 0xF;  // Source A
    uint16_t rbimm =  instr        & 0xF;  // Source B or immediate

    switch (op) {
        // RRR‑type ALU
        case 0x0: sprintf(out, "ADD  R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x1: sprintf(out, "SUB  R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x2: sprintf(out, "AND  R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x3: sprintf(out, "OR   R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x4: sprintf(out, "XOR  R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x5: sprintf(out, "DIVMOD R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x6: sprintf(out, "MUL  R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x7: sprintf(out, "CMP  R%u,R%u,R%u", rd, ra, rbimm); break;

        // RR‑type shift/rotate (opcode=8)
        case 0x8: {
            const char *mn = (rd == 0) ? "LSL"
                               : (rd == 1) ? "LSR"
                               : (rd == 2) ? "ROL"
                                           : "ROR";
            // here 'ra' is the source, 'rbimm' is the count
            sprintf(out, "%s R%u,R%u,%u", mn, rbimm, ra, rbimm);
            break;
        }

        // RRI‑type loads/stores and branches
        case 0x9: sprintf(out, "LW   R%u,[R%u+%u]", rd, ra, rbimm);   break;
        case 0xA: sprintf(out, "SW   [R%u+%u],R%u", ra, rbimm, rd);   break;
        case 0xB: sprintf(out, "BEQ  R%u,R%u,%u", rd, ra, rbimm);    break;
        case 0xF: sprintf(out, "BLT  R%u,R%u,%u", rd, ra, rbimm);    break;

        default:
            sprintf(out, "UNKNOWN 0x%X", instr);
    }
}

/**
 * The fetch stage: grab the next word, push the old one into IF/ID, and print.
 */
void fetch_stage(PipelineState *p, uint16_t *prev_instr) {
    // migrate previous fetch down the pipeline
    p->IF_ID_next.valid       = true;
    p->IF_ID_next.instruction = *prev_instr;

    // PC update on branch
    if (branch_taken) {
        registers->R[15] = branch_target_address;
        branch_taken     = false;
    } else {
        registers->R[15] += 1;
    }

    // fetch new word
    uint16_t pc   = registers->R[15];
    uint16_t word = readFromMemory(&dram, pc);

    // populate next IF/ID
    p->IF_ID_next.pc          = pc;
    p->IF_ID_next.instruction = word;

    // for UI
    char txt[64];
    fmt_instr(word, txt);

    // hand off
    *prev_instr = word;

    // emit
    printf("[FETCH] instruction=0x%04X pc=%u\n", word, pc);
    printf("[PIPELINE]FETCH:%s:%u\n", txt, pc);
    fflush(stdout);
}