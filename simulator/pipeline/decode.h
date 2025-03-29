#ifndef DECODE_H
#define DECODE_H

#include "../pipeline.h"

bool decode_ready(PipelineState* pipeline);
void decode_stage(PipelineState* pipeline);

#endif