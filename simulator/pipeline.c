#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "pipeline.h"
#include "pipeline/fetch.h"
#include "pipeline/decode.h"
// #include "pipeline/execute.h"
// #include "pipeline/memory_stage.h"
// #include "pipeline/writeback.h"

bool pipeline_ready(PipelineState* pipeline) {
    return fetch_ready(pipeline) && decode_ready(pipeline);
}

void pipeline_step(PipelineState* pipeline, uint16_t* value) {
    // Only proceed if pipeline is ready
    //if (!pipeline_ready(pipeline)) {
    //    return;
    // }

    // Execute stages in reverse order (writeback to fetch)
    // writeback_stage(pipeline);
    // memory_stage(pipeline);
    // execute_stage(pipeline);
    decode_stage(pipeline);
    fetch_stage(pipeline, value);

    // Set the values of registers to the 'updated' values in next after each cycle.
    pipeline->IF_ID = pipeline->IF_ID_next;
    pipeline->ID_EX = pipeline->ID_EX_next;
}
