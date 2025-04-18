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
    // 1) Drain the tail of the pipeline
    write_back(pipeline);
    memory_access(pipeline);

    // 2) Detect a RAW hazard between EX/MEM → ID/EX
    bool raw_stall = false;
    if (pipeline->EX_MEM.valid && pipeline->ID_EX.valid) {
        uint16_t dest = pipeline->EX_MEM.regD;
        if (dest != 0 &&
            (dest == pipeline->ID_EX.regA || dest == pipeline->ID_EX.regB)) {
            raw_stall = true;
            printf("[PIPELINE_STALL] RAW hazard on R%u; stalling 1 cycle\n", dest);
        }
    }

    // 3a) If DRAM/cache is busy, stall everything except WB/MEM_WB
    if (memory_operation_in_progress) {
        pipeline->WB     = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
        printf("[PIPELINE_STALL] Memory operation in progress; stalling IF/ID, ID/EX, EX/MEM\n");
    }
    // 3b) Else if RAW hazard, bubble EX/MEM, freeze IF/ID & ID/EX, but still retire WB/MEM_WB
    else if (raw_stall) {
        pipeline->EX_MEM.valid = false;            // turn EX/MEM into a bubble
        pipeline->WB     = pipeline->WB_next;
        pipeline->MEM_WB = pipeline->MEM_WB_next;
    }
    // 3c) Otherwise, normal 5‑stage advance
    else {
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
            if (!busy) {
                fetch_stage(pipeline, value);
            } else {
                pipeline->IF_ID_next.valid = false; // hold bubble
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
