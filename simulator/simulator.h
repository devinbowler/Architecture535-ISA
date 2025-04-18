#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdbool.h>
#include <stdint.h>
#include "memory.h"
#include "pipeline.h"

// Global variable declarations
extern REGISTERS *registers;
extern DRAM dram;
extern Cache *cache;
extern PipelineState pipeline;

// Function declarations
void init_system();
void executeInstructions();
bool pipeline_empty(PipelineState* pipeline);
void stepInstructions();
void storeInstruction(const char *command);

#endif // SIMULATOR_H 