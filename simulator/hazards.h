#ifndef DATA_HAZARDS_H
#define DATA_HAZARDS_H

#include "pipeline.h"
#include <stdbool.h>

typedef struct {
    bool detected;              // Whether a hazard is detected
    bool requires_stall;        // Whether hazard requires stalling
    uint16_t stall_cycles;      // How many cycles to stall
    uint16_t source_reg;        // Register causing the hazard
    uint16_t target_reg;        // Register needing the value
    uint16_t source_stage;      // Pipeline stage with the value (1=EX, 2=MEM, 3=WB)
    uint16_t forwarded_value;   // Value to forward
} HazardInfo;

HazardInfo detect_hazards(PipelineState *pipeline);
void resolve_hazards(PipelineState *pipeline, HazardInfo *hazard);
void forward_result(PipelineState *pipeline, HazardInfo *hazard);

#endi