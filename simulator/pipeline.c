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

extern REGISTERS *registers;
extern bool branch_taken;
extern bool memory_operation_in_progress;

void pipeline_step(PipelineState* pipeline, uint16_t* value) {
    // 1) write‑back & memory
    write_back(pipeline);
    memory_access(pipeline);

    // 2) if memory is busy, stall IF/ID, ID/EX and EX/MEM
    if (memory_operation_in_progress) {
        pipeline->WB     = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        printf("[PIPELINE_STALL] Memory op in progress; stalling.\n");
    } else {
        // 3) normal overlap: EX, ID, IF
        execute(pipeline);
        decode_stage(pipeline);
        if (PIPELINE_ENABLED) {
            fetch_stage(pipeline, value);
        } else {
            bool busy = pipeline->IF_ID.valid  ||
                        pipeline->ID_EX.valid  ||
                        pipeline->EX_MEM.valid ||
                        pipeline->MEM_WB.valid ||
                        memory_operation_in_progress;
            if (!busy)
                fetch_stage(pipeline, value);
            else
                pipeline->IF_ID_next.valid = false;
        }

        // 4) commit all next‐states
        pipeline->WB     = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        pipeline->EX_MEM = pipeline->EX_MEM_next;
        pipeline->ID_EX  = pipeline->ID_EX_next;
        pipeline->IF_ID  = pipeline->IF_ID_next;
    }

    // 5) clear next‐state
    memset(&pipeline->WB_next,     0, sizeof pipeline->WB_next);
    memset(&pipeline->MEM_WB_next, 0, sizeof pipeline->MEM_WB_next);
    memset(&pipeline->EX_MEM_next, 0, sizeof pipeline->EX_MEM_next);
    memset(&pipeline->ID_EX_next,  0, sizeof pipeline->ID_EX_next);
    memset(&pipeline->IF_ID_next,  0, sizeof pipeline->IF_ID_next);

    // 6) handle branch flush if needed
    if (branch_taken && !memory_operation_in_progress) {
        pipeline->IF_ID.valid = false;
        pipeline->ID_EX.valid = false;
        branch_taken = false;
        printf("[PIPELINE] Branch flush: pipeline cleared.\n");
    } else if (branch_taken) {
        printf("[PIPELINE] Branch pending: awaiting memory completion.\n");
    }
}