#include "write_back.h"
#include "../pipeline.h"
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
  
  // Print write back info
  printf("[WRITE-BACK] opcode=%u rd=%u result=%u\n", 
         opcode, regD, result);
  
  if (opcode == 0b0000 || // ADD
        opcode == 0b0001 || // SUB
        opcode == 0b0010 || // AND
        opcode == 0b0011 || // OR
        opcode == 0b0100 || // XOR
        opcode == 0b0101 || // DIVMOD
        opcode == 0b0110 || // MUL
        opcode == 0b1000 || // SHIFT
        opcode == 0b1001) { // LW
        registers->R[regD] = result;
        printf("[WRITE-BACK] Wrote value %u to R%u\n", result, regD);
        if (opcode == 0b0101) { // DIVMOD
            registers->R[regB] = pipeline->MEM_WB.resMod;
            printf("[WRITE-BACK] Wrote remainder %u to R%u\n", pipeline->MEM_WB.resMod, regB);
        }
    }
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
