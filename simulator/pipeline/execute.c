#include <stdio.h>
#include <stdlib.h>
#include "execute.h"
#include "../pipeline.h"
#include "../memory.h"

extern REGISTERS *registers;

// Add global variables to track branch status
bool branch_taken = false;
uint16_t branch_target_address = 0;

// Function to flush the pipeline
void flush_pipeline(PipelineState *pipeline) {
  // Invalidate earlier pipeline stages
  pipeline->IF_ID_next.valid = false;
  pipeline->ID_EX_next.valid = false;
  
  printf("[EXECUTE_BRANCH] Flushing pipeline\n");
}

/**
 * @brief Implements the execute stage of the pipeline. This includes ALU operations and address calculations
 * @param pipeline the pipeline
 */
void execute(PipelineState *pipeline) {
  // if(!execute_ready(pipeline)) return;
  uint16_t PC = pipeline->ID_EX.pc;
  uint16_t regD = pipeline->ID_EX.regD;
  uint16_t regA = pipeline->ID_EX.regA;
  uint16_t regB = pipeline->ID_EX.regB;
  uint16_t imm = pipeline->ID_EX.imm;
  uint16_t opcode = pipeline->ID_EX.opcode;
  uint16_t result = 0;
  
  // Reset branch taken flag at the beginning of execution
  branch_taken = false;
  
  // Get actual register values
  uint16_t valA = registers->R[regA];
  uint16_t valB = registers->R[regB];
  uint16_t valD = registers->R[regD];
  
  pipeline->EX_MEM_next.valid = true;
  
  // Pass key information to next stage
  pipeline->EX_MEM_next.regD = regD;
  pipeline->EX_MEM_next.regA = regA;
  pipeline->EX_MEM_next.regB = regB;
  pipeline->EX_MEM_next.opcode = opcode;
  
  // Print execute info with actual register values
  printf("[EXECUTE] opcode=%u rd=%u ra=%u(val=%u) rb=%u(val=%u)\n", 
         opcode, regD, regA, valA, regB, valB);
  
  // Generate stage information for pipeline visualization
  char instruction_text[50] = "Unknown";
  
  switch(opcode) {
    case 0: // ADD
      result = valA + valB;
      sprintf(instruction_text, "ADD R%d, R%d, R%d", regD, regA, regB);
      printf("[EXECUTE_ADD] R%u = R%u(%u) + R%u(%u) = %u\n", 
             regD, regA, valA, regB, valB, result);
      break;
    case 1: // ADDI
      result = valA + imm;
      sprintf(instruction_text, "ADDI R%d, R%d, %d", regD, regA, imm);
      printf("[EXECUTE_ADDI] R%u = R%u(%u) + %u = %u\n", 
             regD, regA, valA, imm, result);
      break;
    case 2: // NAND
      result = ~(valA & valB);
      sprintf(instruction_text, "NAND R%d, R%d, R%d", regD, regA, regB);
      printf("[EXECUTE_NAND] R%u = ~(R%u(%u) & R%u(%u)) = %u\n", 
             regD, regA, valA, regB, valB, result);
      break;
    case 3: // LUI
      result = imm << 12;
      sprintf(instruction_text, "LUI R%d, %d", regD, imm);
      printf("[EXECUTE_LUI] R%u = %u << 12 = %u\n", 
             regD, imm, result);
      break;
    case 4: // SW
      // Calculate the memory offset that will be added to DATA_SPACE (500) in memory stage
      // For "SW Rd, Ra, imm" - we want to calculate Ra + imm exactly
      result = valA + imm;
      
      // Make sure we're using the full 16-bit value
      sprintf(instruction_text, "SW R%d, R%d, %d", regD, regA, imm);
      printf("[EXECUTE_SW] MEM[R%u(%u) + %u] = R%u(%u) => offset=%u (will access address %u)\n", 
            regA, valA, imm, regD, registers->R[regD], result, result + DATA_SPACE);
      break;
    case 5: // LW
      // Calculate the memory offset that will be added to DATA_SPACE (500) in memory stage
      // For "LW Rd, Ra, imm" - we want to calculate Ra + imm exactly
      result = valA + imm;
      
      // Make sure we're using the full 16-bit value
      sprintf(instruction_text, "LW R%d, R%d, %d", regD, regA, imm);
      printf("[EXECUTE_LW] R%u = MEM[R%u(%u) + %u] => offset=%u (will access address %u)\n", 
            regD, regA, valA, imm, result, result + DATA_SPACE);
      break;
    case 11: // BEQ (0b1011) - Updated from 6 to 11 to match assembler
      // Check if branch condition is true
      if (valD == valA) {
        // Branch is taken, set the target PC and flag
        result = PC + imm; // Jump to PC + immediate
        branch_taken = true;
        branch_target_address = result;
        
        // Flush the pipeline - invalidate earlier stages
        flush_pipeline(pipeline);
        
        printf("[EXECUTE_BEQ] Branch taken: R%u(%u) == R%u(%u), PC = %u + %u = %u\n", 
               regD, valD, regA, valA, PC, imm, result);
      } else {
        // Branch not taken, continue to next instruction
        result = PC + 1;
        printf("[EXECUTE_BEQ] Branch not taken: R%u(%u) != R%u(%u), PC = %u + 1 = %u\n", 
               regD, valD, regA, valA, PC, result);
      }
      sprintf(instruction_text, "BEQ R%d, R%d, %d", regD, regA, imm);
      break;
    case 7: // JALR
      result = PC + 1;
      pipeline->EX_MEM_next.resMod = valA;
      
      // Set branch taken flag and target address
      branch_taken = true;
      branch_target_address = valA;
      
      // Flush the pipeline
      flush_pipeline(pipeline);
      
      sprintf(instruction_text, "JALR R%d, R%d", regD, regA);
      printf("[EXECUTE_JALR] R%u = PC+1 = %u, PC = R%u(%u)\n", 
             regD, result, regA, valA);
      break;
    default: // NOOP or unknown
      printf("[EXECUTE] Unknown opcode: %u\n", opcode);
      sprintf(instruction_text, "UNKNOWN opcode=%d", opcode);
  }
  
  // Report pipeline state for UI
  if (!pipeline->ID_EX.valid || pipeline->ID_EX.opcode == 0) {
    printf("[PIPELINE]EXECUTE:Bubble:%d\n", PC);
  } else {
    printf("[PIPELINE]EXECUTE:%s:%d\n", instruction_text, PC);
  }
  
  pipeline->EX_MEM_next.res = result;
}

/**
 * @brief 
 * @param pipeline the pipeline
 * @return true if the execute stage is ready
 * @return false if the execute stage is not ready
 */
bool execute_ready(PipelineState *pipeline) {
    return pipeline->EX_MEM.valid;
}
