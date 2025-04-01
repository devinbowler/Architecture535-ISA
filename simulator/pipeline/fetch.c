// This file will look at the PC and do a memory access to get the next instruction.
#include <stdio.h>
#include <stdlib.h>
#include "fetch.h"
#include "../memory.h"

extern DRAM dram;
extern REGISTERS* registers;

bool fetch_ready(PipelineState* pipeline) {
    return pipeline->IF_ID.valid;  // Ready to fetch when IF_ID is empty
}

void fetch_stage(PipelineState* pipeline, uint16_t* value) {
    // if (!fetch_ready(pipeline)) return;

    // If there is something to fetch, mark stage as valid
    pipeline->IF_ID_next.valid = true;
    pipeline->IF_ID_next.instruction = *value;
    pipeline->IF_ID_next.pc = registers->R[15];

    // Print fetch info
    printf("[FETCH] instruction=%u pc=%u\n", *value, registers->R[15]);
    fflush(stdout);
}