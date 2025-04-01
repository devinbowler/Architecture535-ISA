#include "memory_access.h"
extern DRAM dram;
extern Cache *cache;
extern REGISTERS *registers;

/**
 * @brief Implements the memory access stage of the pipeline. This will just use our existing cache and DRAM functions
 * @param pipeline the pipeline
 */
void memory_access(PipelineState *pipeline) {
  if(!memory_ready(pipeline)) return;
  pipeline->MEM_WB_next.valid = true;
  pipeline->MEM_WB_next.pc = pipeline->EX_MEM.pc;
  uint16_t opcode = pipeline->EX_MEM.opcode;
  uint16_t regD = pipeline->EX_MEM.regD;
  uint16_t address = pipeline->EX_MEM.res;
  uint16_t value = 0;
  if(opcode == 0b1001) { //LW
    value = read_cache(cache, &dram, address);
    pipeline->MEM_WB_next.res = value;
    printf("[MEMORY] LW: Read value %u from address %u for R%u\n", value, address, regD);
  } else if(opcode == 0b1010) { // SW
    write_through(cache, &dram, address, regD);
    printf("[MEMORY] SW: Wrote value %u to address %u\n", regD, address);
  }
  pipeline->MEM_WB_next.res = address;
  pipeline->MEM_WB_next.resMod = pipeline->EX_MEM.resMod;
  pipeline->MEM_WB_next.opcode = opcode;
  pipeline->MEM_WB_next.regD = regD;
  pipeline->MEM_WB_next.regB = pipeline->EX_MEM.regB;
  fflush(stdout);
}

/**
 * @brief 
 * @param pipeline the pipeline
 * @return true if the memory stage is ready
 * @return false if the memory stage is not ready
 */
bool memory_ready(PipelineState *pipeline) {
  return pipeline->MEM_WB.valid;
}