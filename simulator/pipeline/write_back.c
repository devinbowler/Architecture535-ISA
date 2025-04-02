#include "write_back.h"
#include "pipeline.h"
#include "../memory.h"

extern REGISTERS *registers;

/**
 * @brief Implements the write back stage of the pipeline.
 * @param pipeline the pipeline
 */
void write_back(PipelineState *pipeline) {
  // if (!write_back_ready(pipeline)) return;
  uint16_t opcode = pipeline->MEM_WB.opcode;
  uint16_t regD = pipeline->MEM_WB.regD;
  uint16_t regB = pipeline->MEM_WB.regB;
  uint16_t result = pipeline->MEM_WB.res;
  uint16_t resMod = pipeline->MEM_WB.resMod;
  
  // Print write back info
  printf("[WRITE-BACK] opcode=%u rd=%u result=%u\n", 
         opcode, regD, result);
  
  // Based on opcode, determine which register updates should happen
  if (opcode >= 0b0000 && opcode <= 0b0111) {
    // ALU operations (0000-0111)
    printf("[WB_ALU] Updating R%u = %u (previous=%u)\n", regD, result, registers->R[regD]);
    registers->R[regD] = result;
    // This [REG] format is critical for the UI
    printf("[REG]%d:%d\n", regD, result);
    fflush(stdout);
    
    // DIVMOD produces two results
    if (opcode == 0b0101) {
        registers->R[regB] = resMod;
        printf("[WB_DIVMOD] Also updating R%u = %u\n", regB, resMod);
        printf("[REG]%d:%d\n", regB, resMod);
        fflush(stdout);
    }
  }
  else if (opcode == 0b1000) {
    // SHIFT operations
    printf("[WB_SHIFT] Updating R%u = %u\n", regD, result);
    registers->R[regD] = result;
    printf("[REG]%d:%d\n", regD, result);
    fflush(stdout);
  }
  else if (opcode == 0b1001) {
    // LW instruction writes loaded value to destination register
    printf("[WB_LOAD] Updating R%u = %u (from memory)\n", regD, result);
    registers->R[regD] = result;
    printf("[REG]%d:%d\n", regD, result);
    fflush(stdout);
  }
  
  // SW, BEQ, and BLT don't update registers in write-back
  
  fflush(stdout);
}

/**
 * @brief 
 * @param pipeline the pipeline
 * @return true if the write back stage is ready
 * @return false if the write back stage is not ready
 */
bool write_back_ready(PipelineState *pipeline) {
  return pipeline->WB.valid;
}
