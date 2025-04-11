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
extern bool memory_operation_in_progress;

void pipeline_step(PipelineState* pipeline, uint16_t* value) {
    // ----------------- STAGE COMPUTATION -----------------
    // Always run the write-back and memory-access stages first.
    write_back(pipeline);
    memory_access(pipeline);

    // ----------------- STALL HANDLING FOR MEMORY -----------------
    if (memory_operation_in_progress) {
        // Memory stage is busy (writing to DRAM or updating cache).
        // In this case, stall the earlier stages so that we don’t commit new instructions.
        // Only update the later stages (WB and MEM/WB) so that the ongoing memory operation
        // can complete without interference.
        pipeline->WB    = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        printf("[PIPELINE_STALL] Memory operation in progress; stalling execute, decode, and fetch stages.\n");
    } else {
        // When no memory stall is present, run the remaining stages in order.
        execute(pipeline);
        decode_stage(pipeline);
        fetch_stage(pipeline, value);

        // Commit all pipeline stage next states.
        pipeline->WB    = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        pipeline->EX_MEM = pipeline->EX_MEM_next;
        pipeline->ID_EX  = pipeline->ID_EX_next;
        pipeline->IF_ID  = pipeline->IF_ID_next;
    }

    // ----------------- CLEAR NEXT-STATE REGISTERS -----------------
    memset(&pipeline->WB_next,     0, sizeof(pipeline->WB_next));
    memset(&pipeline->MEM_WB_next, 0, sizeof(pipeline->MEM_WB_next));
    memset(&pipeline->EX_MEM_next, 0, sizeof(pipeline->EX_MEM_next));
    memset(&pipeline->ID_EX_next,  0, sizeof(pipeline->ID_EX_next));
    memset(&pipeline->IF_ID_next,  0, sizeof(pipeline->IF_ID_next));

    // ----------------- BRANCH HANDLING -----------------
    // Two cases:
    // 1) If a branch is taken and there's no memory operation in progress,
    //    then we flush the pipeline immediately.
    // 2) If a branch is taken but memory is still busy, we delay flushing until
    //    the memory stage completes its operation.
    if (branch_taken && !memory_operation_in_progress) {
        // Flush all stages by marking them as bubbles.
        pipeline->IF_ID.valid  = false;
        pipeline->ID_EX.valid  = false;
        pipeline->EX_MEM.valid = false;
        pipeline->MEM_WB.valid = false;
        pipeline->WB.valid     = false;

        // Reset the branch flag after flushing.
        branch_taken = false;
        printf("[PIPELINE] Branch detected and flushed: pipeline cleared.\n");
    } else if (branch_taken && memory_operation_in_progress) {
        // Branch detected but memory op is still ongoing.
        // We delay the flush until the memory operation is complete.
        printf("[PIPELINE] Branch detected but memory stage busy; awaiting memory completion to flush branch.\n");
    }
}
