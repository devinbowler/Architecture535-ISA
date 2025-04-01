#include "memory_access.h"
extern DRAM dram;
extern Cache *cache;
extern REGISTERS *registers;

// Function declaration
bool memory_access_ready(PipelineState *pipeline);

/**
 * @brief Implements the memory access stage of the pipeline. This will just use our existing cache and DRAM functions
 * @param pipeline the pipeline
 */
void memory_access(PipelineState *pipeline) {
    // if (!memory_access_ready(pipeline)) return;
    uint16_t opcode = pipeline->EX_MEM.opcode;
    uint16_t regD = pipeline->EX_MEM.regD;
    uint16_t regB = pipeline->EX_MEM.regB;
    uint16_t result = pipeline->EX_MEM.res;
    uint16_t resMod = 0;
    pipeline->MEM_WB_next.valid = true;
    pipeline->MEM_WB_next.regD = regD;
    pipeline->MEM_WB_next.res = result;
    pipeline->MEM_WB_next.resMod = resMod;

    // Print memory access info
    printf("[MEMORY] opcode=%u rd=%u rb=%u result=%u\n", 
           opcode, regD, regB, result);
    fflush(stdout);

    if (opcode == 0b1001) { // LW
        pipeline->MEM_WB_next.res = readFromMemory(&dram, result);
    }
    else if (opcode == 0b1010) { // SW
        writeToMemory(&dram, result, regB);
    }
}

/**
 * @brief 
 * @param pipeline the pipeline
 * @return true if the memory stage is ready
 * @return false if the memory stage is not ready
 */
bool memory_access_ready(PipelineState *pipeline) {
  return pipeline->MEM_WB.valid;
}