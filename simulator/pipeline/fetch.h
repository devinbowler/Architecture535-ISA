#ifndef FETCH_H
#define FETCH_H

#include "../pipeline.h"

bool fetch_ready(PipelineState* pipeline);
void fetch_stage(PipelineState* pipeline, uint16_t* value);

#endif