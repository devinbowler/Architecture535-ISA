
// fetch.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "fetch.h"
#include "memory.h"
#include "pipeline.h"
#include "globals.h"

extern DRAM        dram;
extern REGISTERS  *registers;
extern Cache      *cache;
extern bool        branch_taken;

// one-shot flag to squash the next fetch completion
static bool fetch_squash_pending = false;

// State variables for fetch memory access
bool fetch_memory_busy = false;
uint16_t fetch_delay_counter = 0;
uint16_t fetch_delay_target = 0;
uint16_t fetch_pending_address = 0;

/**
 * Decode a raw 16-bit instruction into a display string.
 */
static void fmt_instr(uint16_t instr, char *out) {
    if (instr == 0) {
        sprintf(out, "NOP");
        return;
    }
    uint16_t op    = (instr >> 12) & 0xF;
    uint16_t rd    = (instr >>  8) & 0xF;
    uint16_t ra    = (instr >>  4) & 0xF;
    uint16_t rbimm =  instr        & 0xF;
    switch (op) {
        case 0x0: sprintf(out, "ADD  R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x1: sprintf(out, "SUB  R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x2: sprintf(out, "AND  R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x3: sprintf(out, "OR   R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x4: sprintf(out, "XOR  R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x5: sprintf(out, "DIVMOD R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x6: sprintf(out, "MUL  R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x7: sprintf(out, "CMP  R%u,R%u,R%u", rd, ra, rbimm); break;
        case 0x8: {
            const char *mn = (rd==0)?"LSL":(rd==1)?"LSR":(rd==2)?"ROL":"ROR";
            sprintf(out, "%s R%u,R%u,%u", mn, rbimm, ra, rbimm);
            break;
        }
        case 0x9:  sprintf(out, "LW   R%u,[R%u+%u]", rd, ra, rbimm); break;
        case 0xA:  sprintf(out, "SW   [R%u+%u],R%u", ra, rbimm, rd); break;
        case 0xB:  sprintf(out, "BEQ  R%u,R%u,%u", rd, ra, rbimm); break;
        case 0xF:  sprintf(out, "BLT  R%u,R%u,%u", rd, ra, rbimm); break;
        default:   sprintf(out, "UNKNOWN 0x%X", instr);
    }
}

/**
 * The fetch stage: grab the next word, push the old one into IF/ID, and print.
 * Implements memory delay logic and a one-shot squash of the next instruction fetched after a branch enters EX stage.
 */
void fetch_stage(PipelineState *p, uint16_t *prev_instr) {
    char txt[64] = "FETCH waiting";
    char formatted[48];
    uint16_t pc = registers->R[15];

    // Detect a branch in the EX stage (ID_EX pipeline register) and schedule one squash
    if (p->ID_EX.valid && (p->ID_EX.opcode == 0xB || p->ID_EX.opcode == 0xF) && !fetch_squash_pending) {
        fetch_squash_pending = true;
        // Debug log:
        printf("[FETCH] Scheduled squash for next fetch due to branch at PC=%u\n", p->ID_EX.pc);
    }

    // 1. If a memory operation is already in progress, tick the countdown
    if (fetch_memory_busy) {
        fetch_delay_counter++;
        printf("[FETCH_DELAY] Cycle %u of %u\n", fetch_delay_counter, fetch_delay_target);

        // Delay complete?
        if (fetch_delay_counter >= fetch_delay_target) {
            uint16_t word = readFromMemory(&dram, fetch_pending_address);

            if (fetch_squash_pending) {
                // squash this one
                p->IF_ID_next.valid       = true;
                p->IF_ID_next.squashed    = true;
                p->IF_ID_next.pc          = fetch_pending_address;
                p->IF_ID_next.instruction = word;
                fmt_instr(word, formatted);
                snprintf(txt, sizeof(txt), "SQUASHED %s", formatted);
                printf("[FETCH] PC=%u squashed (flush)\n", fetch_pending_address);
                fetch_squash_pending = false;
            } else {
                // normal
                p->IF_ID_next.valid       = true;
                p->IF_ID_next.squashed    = false;
                p->IF_ID_next.pc          = fetch_pending_address;
                p->IF_ID_next.instruction = word;
                fmt_instr(word, txt);
                registers->R[15]++;
                printf("[FETCH] inst=0x%04X pc=%u (after %u cycles)\n",
                       word, fetch_pending_address, fetch_delay_target);
            }
            *prev_instr = word;
            fetch_memory_busy   = false;
            fetch_delay_counter = 0;
        } else {
            // still waiting, bubble fetch
            p->IF_ID_next.valid = false;
            snprintf(txt, sizeof(txt), "FETCH waiting (%u/%u)", fetch_delay_counter, fetch_delay_target);
            printf("[FETCH] waiting %u/%u cycles\n", fetch_delay_counter, fetch_delay_target);
        }
    }
    // 2. Otherwise, start a new fetch
    else {
        if (branch_taken) {
            // bubble IF/ID while branch in-flight
            p->IF_ID_next.valid = false;
            snprintf(txt, sizeof(txt), "FETCH bubble (branch pending)");
            printf("[FETCH] branch pending, bubble\n");
        } else {
            // normal fetch issue
            fetch_pending_address = pc;
            bool hit = false;
            if (CACHE_ENABLED && cache) {
                uint16_t idx = (pc / BLOCK_SIZE) % cache->num_sets;
                uint16_t tag = pc / (BLOCK_SIZE * cache->num_sets);
                for (int i = 0; i < cache->mode; i++) {
                    if (cache->sets[idx].lines[i].valid && cache->sets[idx].lines[i].tag == tag) {
                        hit = true;
                        break;
                    }
                }
            }
            fetch_delay_target = (CACHE_ENABLED && cache && hit) ? USER_CACHE_DELAY : USER_DRAM_DELAY;

            if (fetch_delay_target > 0) {
                fetch_memory_busy   = true;
                fetch_delay_counter = 0;
                p->IF_ID_next.valid = false;
                snprintf(txt, sizeof(txt), "FETCH waiting (0/%u)", fetch_delay_target);
                printf("[FETCH] start memory at PC=%u delay=%u\n", pc, fetch_delay_target);
            } else {
                uint16_t word = readFromMemory(&dram, pc);

                if (fetch_squash_pending) {
                    p->IF_ID_next.valid       = true;
                    p->IF_ID_next.squashed    = true;
                    p->IF_ID_next.pc          = pc;
                    p->IF_ID_next.instruction = word;
                    fmt_instr(word, formatted);
                    snprintf(txt, sizeof(txt), "SQUASHED %s", formatted);
                    printf("[FETCH] PC=%u squashed (flush)\n", pc);
                    fetch_squash_pending = false;
                } else {
                    p->IF_ID_next.valid       = true;
                    p->IF_ID_next.squashed    = false;
                    p->IF_ID_next.pc          = pc;
                    p->IF_ID_next.instruction = word;
                    fmt_instr(word, txt);
                    registers->R[15]++;
                    printf("[FETCH] inst=0x%04X pc=%u immediate\n", word, pc);
                }
                *prev_instr = word;
            }
        }
    }

    // emit UI state
    printf("[PIPELINE]FETCH:%s:%u\n", txt, fetch_pending_address);
    fflush(stdout);
}
