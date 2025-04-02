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
    
    // Debug output before pipeline step
    printf("[PIPELINE_STEP_DEBUG] Before: IF_ID.instruction=%u, ID_EX.opcode=%u, EX_MEM.opcode=%u\n",
           pipeline->IF_ID.instruction, pipeline->ID_EX.opcode, pipeline->EX_MEM.opcode);
    
    // Store copies of the current state for all registers
    IF_ID_Register if_id_copy = pipeline->IF_ID;
    ID_EX_Register id_ex_copy = pipeline->ID_EX;
    EX_MEM_Register ex_mem_copy = pipeline->EX_MEM;
    MEM_WB_Register mem_wb_copy = pipeline->MEM_WB;
    WB_Register wb_copy = pipeline->WB;
    
    // Reset all next stages to current values
    pipeline->IF_ID_next = if_id_copy;
    pipeline->ID_EX_next = id_ex_copy;
    pipeline->EX_MEM_next = ex_mem_copy;
    pipeline->MEM_WB_next = mem_wb_copy;
    pipeline->WB_next = wb_copy;
    
    // Execute stages in reverse order
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
    } else {
        // If memory operation is in progress, only update MEM_WB register
        // This allows the memory stage to continue its delay counting
        // while all other stages keep their original values
        pipeline->WB = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        
        // Keep other stages with their original values
        // pipeline->EX_MEM = pipeline->EX_MEM_next; - Don't update
        // pipeline->ID_EX = pipeline->ID_EX_next;   - Don't update
        // pipeline->IF_ID = pipeline->IF_ID_next;   - Don't update
        
        // For the pipeline visualization, we still need to run each stage function
        // but we'll restore the original values afterward
        
        // Execute the functions but ignore their results 
        execute(pipeline);
        decode_stage(pipeline);
        fetch_stage(pipeline, value);
        
        // Restore the original values for the frozen stages
        pipeline->EX_MEM_next = ex_mem_copy;
        pipeline->ID_EX_next = id_ex_copy;
        pipeline->IF_ID_next = if_id_copy;
        
        // Update the registers
        pipeline->EX_MEM = pipeline->EX_MEM_next;
        pipeline->ID_EX = pipeline->ID_EX_next;
        pipeline->IF_ID = pipeline->IF_ID_next;
        
        printf("[PIPELINE_FREEZE] Pipeline frozen during memory operation - preserving all stage values\n");
    }
    
    // Debug output after pipeline step
    printf("[PIPELINE_STEP_DEBUG] After: IF_ID.instruction=%u, ID_EX.opcode=%u, EX_MEM.opcode=%u\n",
           pipeline->IF_ID.instruction, pipeline->ID_EX.opcode, pipeline->EX_MEM.opcode);
}
