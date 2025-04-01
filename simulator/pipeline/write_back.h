#ifndef WRITE_BACK_H
#define WRITE_BACK_H

#include "../pipeline.h"
#include "../memory.h"
#include <stdio.h>
#include <stdlib.h>

void write_back(PipelineState *pipeline);
bool write_back_ready(PipelineState *pipeline);

#endif
