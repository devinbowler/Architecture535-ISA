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
#include "hazards.h"

extern REGISTERS *registers;
extern bool branch_taken;
extern bool memory_operation_in_progress;
extern bool data_hazard_stall;
extern uint16_t stall_cycles_remaining;

void pipeline_step(PipelineState* pipeline, uint16_t* value) {
    // ----------------- STAGE COMPUTATION -----------------
    // Always run the write-back and memory-access stages first.
    write_back(pipeline);
    memory_access(pipeline);

    // ----------------- DATA HAZARD DETECTION -----------------
    // Detect hazards between ID/EX and later stages
    HazardInfo hazard = detect_hazards(pipeline);
    
    // If we detected a hazard that requires forwarding, do it before execute
    if (hazard.detected && !hazard.requires_stall) {
        resolve_hazards(pipeline, &hazard);
    }
    
    // ----------------- STALL HANDLING -----------------
    bool stall_pipeline = false;
    
    // Check if we're currently stalling due to a data hazard
    if (data_hazard_stall) {
        stall_pipeline = true;
        stall_cycles_remaining--;
        printf("[PIPELINE_STALL] Data hazard stall, %u cycles remaining\n", stall_cycles_remaining);
        
        if (stall_cycles_remaining == 0) {
            data_hazard_stall = false;
            stall_pipeline = false;
            printf("[PIPELINE_STALL] Data hazard stall complete\n");
        }
    }
    
    // Memory stall takes precedence over data hazard stall
    if (memory_operation_in_progress) {
        stall_pipeline = true;
        printf("[PIPELINE_STALL] Memory operation in progress; stalling execute, decode, and fetch stages.\n");
    }
    
    // If stalling is required, only update the later stages
    if (stall_pipeline) {
        // Update only the write-back and memory/write-back stages
        pipeline->WB = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
    } else {
        // When no stall is present, run the remaining stages in order.
        execute(pipeline);
        
        // After execute, if a stall is needed, set it up for the next cycle
        if (hazard.detected && hazard.requires_stall) {
            resolve_hazards(pipeline, &hazard);
        }
        
        decode_stage(pipeline);
        fetch_stage(pipeline, value);

        // Commit all pipeline stage next states.
        pipeline->WB = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        pipeline->EX_MEM = pipeline->EX_MEM_next;
        pipeline->ID_EX = pipeline->ID_EX_next;
        pipeline->IF_ID = pipeline->IF_ID_next;
    }

    // ----------------- CLEAR NEXT-STATE REGISTERS -----------------
    memset(&pipeline->WB_next, 0, sizeof(pipeline->WB_next));
    memset(&pipeline->MEM_WB_next, 0, sizeof(pipeline->MEM_WB_next));
    memset(&pipeline->EX_MEM_next, 0, sizeof(pipeline->EX_MEM_next));
    memset(&pipeline->ID_EX_next, 0, sizeof(pipeline->ID_EX_next));
    memset(&pipeline->IF_ID_next, 0, sizeof(pipeline->IF_ID_next));

    // ----------------- BRANCH HANDLING -----------------
    // Two cases:
    // 1) If a branch is taken and there's no stall condition,
    //    then we flush the pipeline immediately.
    // 2) If a branch is taken but memory is still busy or we have a data hazard stall,
    //    we delay flushing until the stall condition resolves.
    if (branch_taken && !stall_pipeline) {
        // Flush all stages by marking them as bubbles.
        pipeline->IF_ID.valid = false;
        pipeline->ID_EX.valid = false;
        pipeline->EX_MEM.valid = false;
        pipeline->MEM_WB.valid = false;
        pipeline->WB.valid = false;

        // Reset the branch flag after flushing.
        branch_taken = false;
        printf("[PIPELINE] Branch detected and flushed: pipeline cleared.\n");
    } else if (branch_taken && stall_pipeline) {
        // Branch detected but pipeline is stalled.
        printf("[PIPELINE] Branch detected but pipeline stalled; awaiting stall resolution to flush branch.\n");
    }
}
