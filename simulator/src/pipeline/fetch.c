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
extern uint16_t    branch_target_address;

// State variables for fetch memory access
bool fetch_memory_busy = false;
uint16_t fetch_delay_counter = 0;
uint16_t fetch_delay_target = 0;
uint16_t fetch_pending_address = 0;

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
 * Now implements memory delay logic similar to the memory_access stage.
 */
void fetch_stage(PipelineState *p, uint16_t *prev_instr) {
    // Prepare instruction text buffer
    char txt[64] = "FETCH waiting";
    uint16_t pc = registers->R[15];
    
    // 1. If a memory operation is already in progress, tick the countdown
    if (fetch_memory_busy) {
        fetch_delay_counter++;
        printf("[FETCH_DELAY] Cycle %u of %u\n", fetch_delay_counter, fetch_delay_target);
        
        // Check if delay is complete
        if (fetch_delay_counter >= fetch_delay_target) {
            // Complete the fetch
            uint16_t word = readFromMemory(&dram, fetch_pending_address);
            
            // Populate the next IF/ID
            p->IF_ID_next.valid = true;
            p->IF_ID_next.pc = fetch_pending_address;
            p->IF_ID_next.instruction = word;
            
            // Format the instruction for UI
            fmt_instr(word, txt);
            
            // Update PC - only increment PC after fetch completes
            if (branch_taken) {
                registers->R[15] = branch_target_address;
                branch_taken = false;
            } else {
                registers->R[15] += 1;
            }
            
            // Hand off value to next stage
            *prev_instr = word;
            
            // Reset fetch state
            fetch_memory_busy = false;
            fetch_delay_counter = 0;
            
            printf("[FETCH] instruction=0x%04X pc=%u (completed after %u cycles)\n", 
                   word, fetch_pending_address, fetch_delay_target);
        } else {
            // Still waiting, bubble the pipeline at the fetch stage
            // but DON'T invalidate the rest of the pipeline!
            // Only update the IF/ID_next register
            p->IF_ID_next.valid = false;
            sprintf(txt, "FETCH waiting (%u/%u)", fetch_delay_counter, fetch_delay_target);
            
            printf("[FETCH] Waiting for memory, %u/%u cycles\n", 
                   fetch_delay_counter, fetch_delay_target);
        }
    }
    // 2. Otherwise, start a new fetch operation with appropriate delay
    else {
        // Get current PC
        fetch_pending_address = pc;
        
        // Determine if it's a cache hit (to set appropriate delay)
        bool hit = false;
        if (CACHE_ENABLED && cache) {
            uint16_t idx = (pc / BLOCK_SIZE) % cache->num_sets;
            uint16_t tag = pc / (BLOCK_SIZE * cache->num_sets);
            for (int i = 0; i < cache->mode; i++) {
                if (cache->sets[idx].lines[i].valid &&
                    cache->sets[idx].lines[i].tag == tag) {
                    hit = true;
                    break;
                }
            }
        }
        
        // Set the delay target based on cache/DRAM
        fetch_delay_target = (CACHE_ENABLED && cache && hit) ? USER_CACHE_DELAY : USER_DRAM_DELAY;
        
        if (fetch_delay_target > 0) {
            // Start the delay countdown
            fetch_memory_busy = true;
            fetch_delay_counter = 0;
            
            // First cycle is cycle 0, so we don't increment counter here
            p->IF_ID_next.valid = false;  // Insert bubble only at the fetch stage
            sprintf(txt, "FETCH waiting (0/%u)", fetch_delay_target);
            
            printf("[FETCH] Starting memory access at PC=%u, delay=%u cycles\n", 
                   pc, fetch_delay_target);
        } else {
            // No delay needed, process immediately
            uint16_t word = readFromMemory(&dram, pc);
            
            // Populate the next IF/ID
            p->IF_ID_next.valid = true;
            p->IF_ID_next.pc = pc;
            p->IF_ID_next.instruction = word;
            
            // Format the instruction for UI
            fmt_instr(word, txt);
            
            // Update PC
            if (branch_taken) {
                registers->R[15] = branch_target_address;
                branch_taken = false;
            } else {
                registers->R[15] += 1;
            }
            
            // Hand off value to next stage
            *prev_instr = word;
            
            printf("[FETCH] instruction=0x%04X pc=%u (immediate)\n", word, pc);
        }
    }
    
    // Always emit the pipeline state for UI
    printf("[PIPELINE]FETCH:%s:%u\n", txt, fetch_pending_address);
    fflush(stdout);
}
