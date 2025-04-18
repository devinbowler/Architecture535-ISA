// pipeline/memory_access.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "memory_access.h"
#include "../globals.h"

extern DRAM      dram;
extern Cache    *cache;
extern REGISTERS *registers;
extern bool      memory_operation_in_progress;

/**
 * Memory stage: handle loads/stores with cache/DRAM latency, bubble or forward others.
 */
void memory_access(PipelineState *pipeline) {
    // if there's no valid instruction coming from EX_MEM, bubble
    if (!pipeline->EX_MEM.valid) {
        pipeline->MEM_WB_next.valid = false;
        printf("[PIPELINE]MEMORY:NOP:%d\n", pipeline->EX_MEM.pc);
        return;
    }

    // prepare to pass most fields into MEM/WB
    pipeline->MEM_WB_next.valid  = true;
    pipeline->MEM_WB_next.pc     = pipeline->EX_MEM.pc;
    pipeline->MEM_WB_next.opcode = pipeline->EX_MEM.opcode;
    pipeline->MEM_WB_next.regD   = pipeline->EX_MEM.regD;
    pipeline->MEM_WB_next.resMod = pipeline->EX_MEM.resMod;

    uint16_t opcode  = pipeline->EX_MEM.opcode;
    uint16_t address = pipeline->EX_MEM.res;  // ALU result
    static bool     busy       = false;
    static uint16_t delay      = 0, target = 0;
    static uint16_t pend_addr, pend_opcode, pend_regD, pend_val;

    char instruction_text[64];

    // 1) If a memory operation is already in flight, tick the countdown
    if (busy) {
        memory_operation_in_progress = true;
        delay++;
        printf("[MEM_DELAY] Cycle %u of %u\n", delay, target);

        if (delay >= target) {
            // complete it
            if (pend_opcode == 0x9 || pend_opcode == 5) {
                // LW
                uint16_t val;
                if (CACHE_ENABLED && cache != NULL) {
                    val = read_cache(cache, &dram, pend_addr);
                } else {
                    val = readFromMemory(&dram, pend_addr);
                }
                pipeline->MEM_WB_next.res = val;
                sprintf(instruction_text, "LW  R%u,[%u] complete", pend_regD, pend_addr);
                printf("[MEM_LOAD_COMPLETE] R%u <= %u from %u\n", pend_regD, val, pend_addr);
                printf("[MEM]%u:%u\n", pend_addr, val);
            } else {
                // SW
                if (CACHE_ENABLED && cache != NULL) {
                    write_through(cache, &dram, pend_addr, pend_val);
                } else {
                    writeToMemory(&dram, pend_addr, pend_val);
                }
                sprintf(instruction_text, "SW  [%u] <= %u complete", pend_addr, pend_val);
                printf("[MEM_STORE_COMPLETE] [%u] <= %u\n", pend_addr, pend_val);
                printf("[MEM]%u:%u\n", pend_addr, pend_val);
            }
            busy = false;
            memory_operation_in_progress = false;
            delay = 0;
        } else {
            // still waiting → bubble
            pipeline->MEM_WB_next.valid = false;
            // but set a generic text so we can see we're waiting
            sprintf(instruction_text, "MEM waiting (%u/%u)", delay, target);
        }
    }
    // 2) Otherwise, if this is a new LW or SW, start it
    else if (opcode == 0x9 || opcode == 5) {
        // Load word
        bool hit = false;
        if (CACHE_ENABLED && cache) {
            uint16_t idx = (address / BLOCK_SIZE) % cache->num_sets;
            uint16_t tag = address / (BLOCK_SIZE * cache->num_sets);
            for (int i = 0; i < cache->mode; i++)
                if (cache->sets[idx].lines[i].valid &&
                    cache->sets[idx].lines[i].tag == tag)
                    hit = true;
        } else {
          target = USER_DRAM_DELAY;
          printf("[MEMORY] Cache disabled, using DRAM access delay of %d cycles\n", USER_DRAM_DELAY);
        }
        target      = (CACHE_ENABLED && cache && hit) ? USER_CACHE_DELAY : USER_DRAM_DELAY;
        pend_addr   = address;
        pend_opcode = opcode;
        pend_regD   = pipeline->EX_MEM.regD;
        busy        = true;
        delay       = 0;
        memory_operation_in_progress = true;
        sprintf(instruction_text, "LW  R%u,[%u] start", pend_regD, pend_addr);
        pipeline->MEM_WB_next.valid = false;
    }
    else if (opcode == 0xA || opcode == 4) {
        // Store word
        uint16_t val = registers->R[pipeline->EX_MEM.regD];
        bool hit = false;
        if (CACHE_ENABLED && cache) {
            uint16_t idx = (address / BLOCK_SIZE) % cache->num_sets;
            uint16_t tag = address / (BLOCK_SIZE * cache->num_sets);
            for (int i = 0; i < cache->mode; i++)
                if (cache->sets[idx].lines[i].valid &&
                    cache->sets[idx].lines[i].tag == tag)
                    hit = true;
        }
        target      = (CACHE_ENABLED && cache && hit) ? USER_CACHE_DELAY : USER_DRAM_DELAY;
        pend_addr   = address;
        pend_opcode = opcode;
        pend_val    = val;
        busy        = true;
        delay       = 0;
        memory_operation_in_progress = true;
        sprintf(instruction_text, "SW  [%u] <= %u start", pend_addr, pend_val);
        pipeline->MEM_WB_next.valid = false;
    }
    // 3) Non-memory op: just forward the ALU result
    else {
        pipeline->MEM_WB_next.res = pipeline->EX_MEM.res;
        memory_operation_in_progress = false;
        sprintf(instruction_text, "ALU    result=%u", pipeline->EX_MEM.res);
    }

    // final pipeline UI print: show exactly what we built above
    printf("[MEMORY] opcode=%u addr=%u\n", opcode, address);
    printf("[PIPELINE]MEMORY:%s:%d\n", instruction_text, pipeline->EX_MEM.pc);
    fflush(stdout);
}
