#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "pipeline.h"
#include "pipeline/fetch.h"
#include "pipeline/decode.h"
#include "pipeline/execute.h"
#include "pipeline/memory_access.h"
#include "pipeline/write_back.h"

typedef struct {
    bool register_in_use[16];  // Assuming 16 registers
    bool functional_unit_in_use[4];  // Assuming 4 functional units
} Scoreboard;

void init_scoreboard(Scoreboard* scoreboard) {
    memset(scoreboard->register_in_use, false, sizeof(scoreboard->register_in_use));
    memset(scoreboard->functional_unit_in_use, false, sizeof(scoreboard->functional_unit_in_use));
}

void issue_instruction(Scoreboard* scoreboard, uint16_t regD, uint16_t functional_unit) {
    scoreboard->register_in_use[regD] = true;
    scoreboard->functional_unit_in_use[functional_unit] = true;
}

void complete_instruction(Scoreboard* scoreboard, uint16_t regD, uint16_t functional_unit) {
    scoreboard->register_in_use[regD] = false;
    scoreboard->functional_unit_in_use[functional_unit] = false;
}

bool can_issue_instruction(Scoreboard* scoreboard, uint16_t regD, uint16_t functional_unit) {
    return !scoreboard->register_in_use[regD] && !scoreboard->functional_unit_in_use[functional_unit];
}

bool pipeline_ready(PipelineState* pipeline) {
    return fetch_ready(pipeline) && decode_ready(pipeline);
}

void pipeline_step(PipelineState* pipeline, uint16_t* value) {
    // Only proceed if pipeline is ready
    //if (!pipeline_ready(pipeline)) {
    //    return;
    // }

    fetch_stage(pipeline, value);
    decode_stage(pipeline);
    // execute_stage(pipeline);
    // memory_stage(pipeline);
    // writeback_stage(pipeline);

    // Set the values of registers to the 'updated' values in next after each cycle.
    pipeline->IF_ID = pipeline->IF_ID_next;
    pipeline->ID_EX = pipeline->ID_EX_next;
}
