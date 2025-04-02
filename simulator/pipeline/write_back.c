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
  uint16_t resMod = pipeline->MEM_WB.resMod;
  
  // Print write back info
  printf("[WRITE-BACK] opcode=%u rd=%u result=%u\n", 
         opcode, regD, result);
  
  // Generate stage information for pipeline visualization
  char instruction_text[50] = "NOP";
  
  // Based on opcode, determine which register updates should happen
  if (opcode >= 0b0000 && opcode <= 0b0111) {
    // ALU operations (0000-0111)
    switch(opcode) {
        case 0b0000: sprintf(instruction_text, "ADD R%d, R%d, R%d = %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.regB, result); break;
        case 0b0001: sprintf(instruction_text, "SUB R%d, R%d, R%d = %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.regB, result); break;
        case 0b0010: sprintf(instruction_text, "AND R%d, R%d, R%d = %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.regB, result); break;
        case 0b0011: sprintf(instruction_text, "OR R%d, R%d, R%d = %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.regB, result); break;
        case 0b0100: sprintf(instruction_text, "XOR R%d, R%d, R%d = %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.regB, result); break;
        case 0b0101: sprintf(instruction_text, "DIVMOD R%d, R%d, R%d = %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.regB, result); break;
        case 0b0110: sprintf(instruction_text, "MUL R%d, R%d, R%d = %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.regB, result); break;
        case 0b0111: sprintf(instruction_text, "CMP R%d, R%d, R%d = %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.regB, result); break;
        default: sprintf(instruction_text, "UNKNOWN ALU op=%d", opcode);
    }
    
    printf("[WB_ALU] Updating R%u = %u (previous=%u)\n", regD, result, registers->R[regD]);
    registers->R[regD] = result;
    printf("[REG]%d:%d\n", regD, result);
    
    // DIVMOD produces two results
    if (opcode == 0b0101) {
        registers->R[regB] = resMod;
        printf("[WB_DIVMOD] Also updating R%u = %u\n", regB, resMod);
        printf("[REG]%d:%d\n", regB, resMod);
    }
  }
  else if (opcode == 0b1000) {
    // SHIFT operations
    sprintf(instruction_text, "SHIFT R%d, R%d, R%d = %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.regB, result);
    printf("[WB_SHIFT] Updating R%u = %u\n", regD, result);
    registers->R[regD] = result;
    printf("[REG]%d:%d\n", regD, result);
  }
  else if (opcode == 0b1001 || opcode == 5) {
    // LW instruction writes loaded value to destination register (both old and new opcodes)
    sprintf(instruction_text, "LW R%d, [R%d + %d] = %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.imm, result);
    printf("[WB_LOAD] Updating R%u = %u (from memory), previous value was %u\n", 
           regD, result, registers->R[regD]);
    registers->R[regD] = result;
    printf("[REG]%d:%d\n", regD, result);
  }
  else if (opcode == 0b1010 || opcode == 4) {
    // SW instruction - storage already completed in memory stage (both old and new opcodes)
    sprintf(instruction_text, "SW [R%d + %d], R%d", pipeline->MEM_WB.regA, pipeline->MEM_WB.imm, regD);  
  }
  else if (opcode == 0b1011) {
    // BEQ instruction
    sprintf(instruction_text, "BEQ R%d, R%d, %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.imm);
  }
  else if (opcode == 0b1111) {
    // BLT instruction
    sprintf(instruction_text, "BLT R%d, R%d, %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.imm);
  }
  else {
    sprintf(instruction_text, "UNKNOWN op=%d", opcode);
  }
  
  // Report pipeline state for UI
  if (!pipeline->MEM_WB.valid || pipeline->MEM_WB.opcode == 0) {
    printf("[PIPELINE]WRITEBACK:Bubble:%d\n", pipeline->MEM_WB.pc);
  } else {
    printf("[PIPELINE]WRITEBACK:%s:%d\n", instruction_text, pipeline->MEM_WB.pc);
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
