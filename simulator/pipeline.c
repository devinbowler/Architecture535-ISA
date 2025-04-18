// pipeline.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "pipeline.h"
#include "memory.h"
#include "globals.h"
#include "pipeline/fetch.h"
#include "pipeline/decode.h"
#include "pipeline/execute.h"
#include "pipeline/memory_access.h"
#include "pipeline/write_back.h"
#include "hazards.h"

extern bool data_hazard_stall;
extern uint16_t stall_cycles_remaining;
extern REGISTERS *registers;
extern bool branch_taken;
extern bool memory_operation_in_progress;

void pipeline_step(PipelineState* pipeline, uint16_t* value) {
    // 1) Drain the tail of the pipeline
    write_back(pipeline);
    memory_access(pipeline);

    // ----------------- DATA HAZARD DETECTION -----------------
    HazardInfo hazard = detect_hazards(pipeline);

    // Always resolve hazards (whether they need stalling or not)
    if (hazard.detected) {
        resolve_hazards(pipeline, &hazard);
    }

    // ----------------- STALL HANDLING -----------------
  // 3a) If DRAM/cache is busy, stall everything except WB/MEM_WB
    if (memory_operation_in_progress) {
        pipeline->WB     = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        printf("[PIPELINE_STALL] Memory operation in progress; stalling IF/ID, ID/EX, EX/MEM\n");
    }
    // 3b) Else if RAW hazard, bubble EX/MEM, freeze IF/ID & ID/EX, but still retire WB/MEM_WB
    else if (data_hazard_stall) {
        pipeline->EX_MEM.valid = false;            // turn EX/MEM into a bubble
        pipeline->WB     = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        stall_cycles_remaining--;
        if(stall_cycles_remaining == 0) {
            data_hazard_stall = false;
        }
    }
    // 3c) Otherwise, normal 5‑stage advance
    else {
        execute(pipeline);
        
        if (PIPELINE_ENABLED) {
            // Normal pipelined operation - run all stages in parallel
            decode_stage(pipeline);
            fetch_stage(pipeline, value);
        } else {
            // Non-pipelined mode: Only one instruction in pipeline at a time
            
            // Check if pipeline is completely empty except possibly for IF/ID
            bool pipeline_empty = !pipeline->ID_EX.valid && 
                                 !pipeline->EX_MEM.valid && 
                                 !pipeline->MEM_WB.valid;
                                 
            if (pipeline_empty) {
                // Pipeline is empty (except maybe IF/ID), so we can advance IF/ID to ID/EX
                if (pipeline->IF_ID.valid) {
                    decode_stage(pipeline);
                    printf("[PIPELINE] Non-pipelined mode: decoding instruction\n");
                    // Don't fetch yet - we just moved an instruction to decode
                } else {
                    // Nothing in IF/ID, and rest of pipeline is empty, safe to fetch
                    fetch_stage(pipeline, value);
                    printf("[PIPELINE] Non-pipelined mode: fetching instruction\n");
                }
            } else {
                // Pipeline not empty yet, hold all stages before execute
                pipeline->ID_EX_next.valid = false;
                pipeline->IF_ID_next.valid = false;
                printf("[PIPELINE] Non-pipelined mode: waiting for pipeline to drain\n");
            }
        }

        // commit all stages
        pipeline->WB     = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        pipeline->EX_MEM = pipeline->EX_MEM_next;
        pipeline->ID_EX  = pipeline->ID_EX_next;
        pipeline->IF_ID  = pipeline->IF_ID_next;
    }

    // 4) Clear next‑state latches
    memset(&pipeline->WB_next,     0, sizeof pipeline->WB_next);
    memset(&pipeline->MEM_WB_next, 0, sizeof pipeline->MEM_WB_next);
    memset(&pipeline->EX_MEM_next, 0, sizeof pipeline->EX_MEM_next);
    memset(&pipeline->ID_EX_next,  0, sizeof pipeline->ID_EX_next);
    memset(&pipeline->IF_ID_next,  0, sizeof pipeline->IF_ID_next);

    // 5) On branch, flush entire pipeline (once memory is done)
    if (branch_taken && !memory_operation_in_progress) {
        pipeline->IF_ID.valid  = false;
        pipeline->ID_EX.valid  = false;
        pipeline->EX_MEM.valid = false;
        pipeline->MEM_WB.valid = false;
        pipeline->WB.valid     = false;
        branch_taken = false;
        printf("[PIPELINE] Branch taken; pipeline flushed\n");
    } else if (branch_taken) {
        printf("[PIPELINE] Branch pending; awaiting memory completion to flush\n");
    }
}
