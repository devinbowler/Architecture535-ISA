#ifndef EXECUTE_H
#define EXECUTE_H

#include "pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern bool branch_taken;
extern uint16_t branch_target_address;

void flush_pipeline(PipelineState *pipeline);
void execute(PipelineState *pipeline);

#endif