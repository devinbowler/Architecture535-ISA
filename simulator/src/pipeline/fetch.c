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
extern bool        branch_taken;
extern uint16_t    branch_target_address;
extern bool        memory_operation_in_progress;
extern Cache      *cache;

// State for fetch memory access
bool fetch_memory_busy = false;
uint16_t fetch_delay_counter = 0;
uint16_t fetch_delay_target = 0;
static uint16_t pending_fetch_addr = 0;
static uint16_t pending_fetch_result = 0;

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
    char txt[64];
    
    // If fetch is busy with a memory operation
    if (fetch_memory_busy) {
        memory_operation_in_progress = true;
        
        // Increment delay counter and check if complete
        fetch_delay_counter++;
        printf("[FETCH_DELAY] Cycle %u/%u\n", fetch_delay_counter, fetch_delay_target);
        
        if (fetch_delay_counter >= fetch_delay_target) {
            // Memory operation complete - get the fetched instruction
            *prev_instr = pending_fetch_result;
            
            // Set up the next pipeline stage
            p->IF_ID_next.valid = true;
            p->IF_ID_next.instruction = *prev_instr;
            p->IF_ID_next.pc = pending_fetch_addr;
            
            // Format instruction for display
            fmt_instr(*prev_instr, txt);
            printf("[FETCH] Complete: 0x%04X from pc=%u\n", *prev_instr, pending_fetch_addr);
            printf("[PIPELINE]FETCH:%s:%u\n", txt, pending_fetch_addr);
            
            // Reset memory operation state
            fetch_memory_busy = false;
            fetch_delay_counter = 0;
            
            // Only clear memory_operation_in_progress if this was the only memory op
            // in progress (memory_access stage might still be busy)
            memory_operation_in_progress = false;
            
            // Only update PC after fetch completes
            if (branch_taken) {
                registers->R[15] = branch_target_address;
                branch_taken = false;
                printf("[FETCH] PC updated to branch target: %u\n", registers->R[15]);
            } else {
                registers->R[15] = pending_fetch_addr + 1;
                printf("[FETCH] PC incremented to: %u\n", registers->R[15]);
            }
        } else {
            // Still waiting for memory access to complete
            p->IF_ID_next.valid = false;
            printf("[PIPELINE]FETCH:WAITING:%u\n", pending_fetch_addr);
        }
    } else {
        // Start a new fetch operation
        uint16_t pc = registers->R[15];
        
        // Check cache hit/miss
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
        
        // Set delay based on cache hit/miss
        fetch_delay_target = (CACHE_ENABLED && cache && hit) ? USER_CACHE_DELAY : USER_DRAM_DELAY;
        
        // Start memory operation
        pending_fetch_addr = pc;
        pending_fetch_result = readFromMemory(&dram, pc);
        fetch_memory_busy = true;
        fetch_delay_counter = 0;
        memory_operation_in_progress = true;
        
        // No valid instruction during first cycle
        p->IF_ID_next.valid = false;
        
        printf("[FETCH] Starting fetch at pc=%u, delay=%u cycles\n", pc, fetch_delay_target);
        printf("[PIPELINE]FETCH:FETCHING:%u\n", pc);
    }
    
    fflush(stdout);
}
