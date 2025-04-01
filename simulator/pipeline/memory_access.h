#ifndef MEMORY_ACCESS_H
#define MEMORY_ACCESS_H

#include "../pipeline.h"
#include "../memory.h"
#include <stdio.h>
#include <stdlib.h>

void memory_access(PipelineState *pipeline, DRAM *dram, Cache *cache);
bool memory_ready(PipelineState *pipeline);

#endif