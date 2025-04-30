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
    uint16_t imm12 =  instr        & 0xFFF;  // For JMP, we use a 12-bit immediate
    
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
        case 0xC:  sprintf(out, "JMP  %u", imm12); break;  // New JMP instruction with 12-bit immediate
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
    // Include JMP (opcode 0xC) in the list of instructions that require squashing
    if (p->ID_EX.valid && (p->ID_EX.opcode == 0xB || p->ID_EX.opcode == 0xF || p->ID_EX.opcode == 0xC) && !fetch_squash_pending) {
        fetch_squash_pending = true;
        // Debug log:
        printf("[FETCH] Scheduled squash for next fetch due to branch/jump at PC=%u\n", p->ID_EX.pc);
    }

    // 1. If a memory operation is already in progress, tick the countdown
    if (fetch_memory_busy) {
        fetch_delay_counter++;
        printf("[FETCH_DELAY] Cycle %u of %u\n", fetch_delay_counter, fetch_delay_target);

        // Delay complete?
        if (fetch_delay_counter >= fetch_delay_target) {
            bool cache_hit = false;
            uint16_t word = 0;
            
            if (CACHE_ENABLED && cache) {
                word = fetch_with_cache(cache, &dram, fetch_pending_address, &cache_hit);
            } else {
                word = readFromMemory(&dram, fetch_pending_address);
            }

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
                printf("[FETCH] inst=0x%04X pc=%u (after %u cycles), cache hit=%s\n",
                       word, fetch_pending_address, fetch_delay_target, 
                       cache_hit ? "true" : "false");
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
            bool cache_hit = false;
            
            if (CACHE_ENABLED && cache) {
                // Check if the instruction is already in the cache
                uint16_t block_offset = pc % BLOCK_SIZE;
                uint16_t block_address = pc - block_offset;
                uint16_t set_index = (block_address / BLOCK_SIZE) % cache->num_sets;
                uint16_t tag = block_address / (BLOCK_SIZE * cache->num_sets);
                
                // Check if this block is in the cache
                Set *set = &cache->sets[set_index];
                for (int i = 0; i < cache->mode; i++) {
                    if (set->lines[i].valid && set->lines[i].tag == tag) {
                        cache_hit = true;
                        break;
                    }
                }
            }
            
            // Set appropriate delay based on whether it's a cache hit or miss
            fetch_delay_target = (CACHE_ENABLED && cache && cache_hit) ? USER_CACHE_DELAY : USER_DRAM_DELAY;

            if (fetch_delay_target > 0) {
                fetch_memory_busy   = true;
                fetch_delay_counter = 0;
                p->IF_ID_next.valid = false;
                snprintf(txt, sizeof(txt), "FETCH waiting (0/%u)", fetch_delay_target);
                printf("[FETCH] start memory at PC=%u delay=%u, cache hit=%s\n", 
                       pc, fetch_delay_target, cache_hit ? "true" : "false");
            } else {
                bool cache_hit = false;
                uint16_t word = 0;
                
                if (CACHE_ENABLED && cache) {
                    word = fetch_with_cache(cache, &dram, pc, &cache_hit);
                } else {
                    word = readFromMemory(&dram, pc);
                }

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
                    printf("[FETCH] inst=0x%04X pc=%u immediate, cache hit=%s\n", 
                           word, pc, cache_hit ? "true" : "false");
                }
                *prev_instr = word;
            }
        }
    }

    // emit UI state
    printf("[PIPELINE]FETCH:%s:%u\n", txt, fetch_pending_address);
    
    // For UI visualization of fetch status
    if (fetch_memory_busy) {
        printf("[FETCH_STATUS]busy:%u:%u\n", fetch_delay_counter, fetch_delay_target);
    }
    
    fflush(stdout);
}
