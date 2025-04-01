#include "write_back.h"

/**
 * @brief Implements the write back stage of the pipeline. This will also just use our memory functions
 * @param pipeline the pipeline
 */
void write_back(PipelineState *pipeline, REGISTERS *registers) {
  if (!write_back_ready(pipeline)) return;
  uint16_t opcode = pipeline->MEM_WB.opcode;
  uint16_t regD = pipeline->MEM_WB.regD;
  uint16_t result = pipeline->MEM_WB.res;  
  if (opcode == 0b0000 || // ADD
        opcode == 0b0001 || // SUB
        opcode == 0b0010 || // AND
        opcode == 0b0011 || // OR
        opcode == 0b0100 || // XOR
        opcode == 0b0101 || // DIVMOD (quotient to regD)
        opcode == 0b0110 || // MUL
        opcode == 0b1000 || // SHIFT
        opcode == 0b1001) { // LW
        
        // Write result to register file
        registers[regD] = result;
        
        printf("[WRITE-BACK] Wrote value %u to R%u\n", result, regD);
        if (opcode == 0b0101) { // DIVMOD
            registers->reg = pipeline->MEM_WB.resMod;
            printf("[WRITE-BACK] Wrote remainder %u to R%u\n", pipeline->MEM_WB.resMod, regD+1);
        }
    }
    complete_instruction(scoreboard, regD, pipeline->MEM_WB.functional_unit);
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
