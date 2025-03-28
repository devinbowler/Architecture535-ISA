#include "fetch.h"
#include "decode.h"
// #include "execute.h"
// #include "memory_stage.h"
// #include "writeback.h"

PipelineState pipeline;

void pipeline_step() {
//    writeback_stage(&pipeline);
//    memory_stage(&pipeline);
//    execute_stage(&pipeline);
    decode_stage(&pipeline);
    fetch_stage(&pipeline);
}
