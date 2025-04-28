#ifndef FETCH_H
#define FETCH_H

#include "pipeline.h"
#include <stdbool.h>
#include <stdint.h>

// Export fetch memory state variables
extern bool fetch_memory_busy;
extern uint16_t fetch_delay_counter;
extern uint16_t fetch_delay_target;

void fetch_stage(PipelineState* pipeline, uint16_t* value);

#endif
