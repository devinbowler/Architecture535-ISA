#ifndef EXECUTE_H
#define EXECUTE_H

#include "../pipeline.h"
#include <stdio.h>
#include <stdlib.h>

void execute(PipelineState *pipeline);
bool execute_ready(PipelineState *pipeline);

#endif
