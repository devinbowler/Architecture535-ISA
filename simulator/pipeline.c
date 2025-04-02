#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "pipeline.h"
#include "memory.h"
#include "pipeline/fetch.h"
#include "pipeline/decode.h"
#include "pipeline/execute.h"
#include "pipeline/memory_access.h"
#include "pipeline/write_back.h"

extern REGISTERS *registers;
extern bool branch_taken;

/*
typedef struct {
    bool busy[16];  // Assuming 16 registers
    bool ready[16];
} Scoreboard;

// Initialize scoreboard
void init_scoreboard(Scoreboard* sb) {
    for(int i = 0; i < 16; i++) {
        sb->busy[i] = false;
        sb->ready[i] = true;
    }
}

// Check if instruction can be issued
bool check_scoreboard(Scoreboard* sb, uint16_t reg) {
    return !sb->busy[reg];
}

// Set register as busy
void set_scoreboard(Scoreboard* sb, uint16_t reg) {
    sb->busy[reg] = true;
    sb->ready[reg] = false;
}

// Clear register as busy
void clear_scoreboard(Scoreboard* sb, uint16_t reg) {
    sb->busy[reg] = false;
    sb->ready[reg] = true;
}

// Complete instruction execution
void complete_instruction(Scoreboard* sb, uint16_t reg, uint16_t functional_unit) {
    (void)functional_unit;  // Suppress unused parameter warning
    clear_scoreboard(sb, reg);
}

// Check if instruction can be issued
bool can_issue_instruction(Scoreboard* sb, uint16_t reg, uint16_t functional_unit) {
    (void)functional_unit;  // Suppress unused parameter warning
    return check_scoreboard(sb, reg);
}

// Issue instruction
void issue_instruction(Scoreboard* sb, uint16_t reg, uint16_t functional_unit) {
    (void)functional_unit;  // Suppress unused parameter warning
    set_scoreboard(sb, reg);
}
*/

// Check if pipeline is ready to proceed
bool pipeline_ready(PipelineState* pipeline) {
    return pipeline->IF_ID.valid && 
           pipeline->ID_EX.valid && 
           pipeline->EX_MEM.valid && 
           pipeline->MEM_WB.valid && 
           pipeline->WB.valid;
}

// Step pipeline one cycle
void pipeline_step(PipelineState* pipeline, uint16_t* value) {
    // Check if there's an ongoing memory operation (memory stage will set this flag)
    extern bool memory_operation_in_progress;
    
    // Execute stages in reverse order to maintain dependencies
    write_back(pipeline);
    memory_access(pipeline);
    
    // Only execute other stages if no memory operation is in progress
    if (!memory_operation_in_progress) {
        execute(pipeline);
        decode_stage(pipeline);
        fetch_stage(pipeline, value);
        
        // Update all pipeline registers
        pipeline->WB = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        pipeline->EX_MEM = pipeline->EX_MEM_next;
        pipeline->ID_EX = pipeline->ID_EX_next;
        pipeline->IF_ID = pipeline->IF_ID_next;
        
        // If a branch was taken, prepare for next cycle's fetch from new target
        if (branch_taken) {
            printf("[PIPELINE_BRANCH] Branch taken, next fetch will be from new target\n");
            // Branch handling is now done in fetch stage
        }
    } else {
        // If memory operation is in progress:
        // 1. Only update WB and MEM_WB registers to allow memory stage to continue
        // 2. Keep all other stages frozen
        pipeline->WB = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        
        // Don't update other pipeline registers to freeze those stages
        printf("[PIPELINE_FREEZE] Pipeline frozen during memory operation\n");
        printf("[PIPELINE_STATE] PC=%u, Memory operation in progress\n", registers->R[15]);
    }
    
    // Clear next stage registers for next cycle
    memset(&pipeline->WB_next, 0, sizeof(pipeline->WB_next));
    memset(&pipeline->MEM_WB_next, 0, sizeof(pipeline->MEM_WB_next));
    memset(&pipeline->EX_MEM_next, 0, sizeof(pipeline->EX_MEM_next));
    memset(&pipeline->ID_EX_next, 0, sizeof(pipeline->ID_EX_next));
    memset(&pipeline->IF_ID_next, 0, sizeof(pipeline->IF_ID_next));
}
