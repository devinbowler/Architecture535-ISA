#include <stdio.h>
#include <stdlib.h>
#include "execute.h"
#include "pipeline.h"
#include "../memory.h"

extern REGISTERS *registers;

/**
 * @brief Implements the execute stage of the pipeline. This includes ALU operations and address calculations
 * @param pipeline the pipeline
 */
void execute(PipelineState *pipeline) {
  // Check if there's a valid instruction to execute
  if (!pipeline->ID_EX.valid || pipeline->ID_EX.opcode == 0) {
    // If no valid instruction to execute, propagate a bubble
    pipeline->EX_MEM_next.valid = true;  // We're valid but empty (bubble)
    pipeline->EX_MEM_next.opcode = 0;    // No operation
    pipeline->EX_MEM_next.pc = pipeline->ID_EX.pc;  // Keep PC for tracking
    
    printf("[EXECUTE] No valid instruction to execute (bubble)\n");
    printf("[PIPELINE]EXECUTE:Bubble:%d\n", pipeline->ID_EX.pc);
    return;
  }
  
  uint16_t PC = pipeline->ID_EX.pc;
  uint16_t regD = pipeline->ID_EX.regD;
  uint16_t regA = pipeline->ID_EX.regA;
  uint16_t regB = pipeline->ID_EX.regB;
  uint16_t imm = pipeline->ID_EX.imm;
  uint16_t opcode = pipeline->ID_EX.opcode;
  uint16_t result = 0;
  
  // Get actual register values
  uint16_t valA = registers->R[regA];
  uint16_t valB = registers->R[regB];
  
  pipeline->EX_MEM_next.valid = true;
  
  // Pass key information to next stage
  pipeline->EX_MEM_next.regD = regD;
  pipeline->EX_MEM_next.regA = regA;
  pipeline->EX_MEM_next.regB = regB;
  pipeline->EX_MEM_next.opcode = opcode;
  pipeline->EX_MEM_next.imm = imm;
  pipeline->EX_MEM_next.pc = PC;
  
  // Print execute info with actual register values
  printf("[EXECUTE] opcode=%u rd=%u ra=%u(val=%u) rb=%u(val=%u)\n", 
         opcode, regD, regA, valA, regB, valB);
  
  // Immediately generate instruction text based on opcode for visualization
  // This ensures we always have instruction text regardless of what happens in the switch
  char instruction_text[50] = "Unknown";
  switch(opcode) {
    case 0b0000: sprintf(instruction_text, "ADD R%d, R%d, R%d", regD, regA, regB); break;
    case 0b0001: sprintf(instruction_text, "SUB R%d, R%d, R%d", regD, regA, regB); break;
    case 0b0010: sprintf(instruction_text, "AND R%d, R%d, R%d", regD, regA, regB); break;
    case 0b0011: sprintf(instruction_text, "OR R%d, R%d, R%d", regD, regA, regB); break;
    case 0b0100: sprintf(instruction_text, "XOR R%d, R%d, R%d", regD, regA, regB); break;
    case 0b0101: sprintf(instruction_text, "DIVMOD R%d, R%d, R%d", regD, regA, regB); break;
    case 0b0110: sprintf(instruction_text, "MUL R%d, R%d, R%d", regD, regA, regB); break;
    case 0b0111: sprintf(instruction_text, "CMP R%d, R%d, R%d", regD, regA, regB); break;
    case 0b1000: sprintf(instruction_text, "SHIFT R%d, R%d, R%d", regD, regA, regB); break;
    case 0b1001: sprintf(instruction_text, "LW R%d, [R%d + %d]", regD, regA, imm); break;
    case 0b1010: sprintf(instruction_text, "SW [R%d + %d], R%d", regA, imm, regD); break;
    case 0b1011: sprintf(instruction_text, "BEQ R%d, R%d, %d", regD, regA, imm); break;
    case 0b1111: sprintf(instruction_text, "BLT R%d, R%d, %d", regD, regA, imm); break;
    default: sprintf(instruction_text, "UNKNOWN opcode=%d", opcode);
  }
  
  // Execute the actual operation by opcode
  switch(opcode) {
    case 0b0000: //ADD
      result = valA + valB;
      printf("[EXECUTE_ADD] R%u = R%u(%u) + R%u(%u) = %u\n", 
             regD, regA, valA, regB, valB, result);
      break;
    case 0b0001: //SUB
      result = valA - valB;
      printf("[EXECUTE_SUB] R%u = R%u(%u) - R%u(%u) = %u\n", 
             regD, regA, valA, regB, valB, result);
      break;
    case 0b0010: //AND
      result = valA & valB;
      break;
    case 0b0011: //OR
      result = valA | valB;
      break;
    case 0b0100: //XOR
      result = valA ^ valB;
      break;
    case 0b0101: //DIVMOD
      result = valA / valB;
      pipeline->EX_MEM_next.resMod = valA % valB;
      break;
    case 0b0110: //MUL
      result = valA * valB;
      break;
    case 0b0111: //CMP
      result = valA - valB;
      break;
    case 0b1000: //SHIFT
      switch(pipeline->ID_EX.type) {
        case(0b0000): //LSL
          result = registers->R[regD] << valA;
          break;
        case(0b0001): //LSR
          result = registers->R[regD] >> valA;
          break;
        case(0b0010): //ROR
          result = (valA << (valB % 16)) | (valA >> (16 - (valB % 16)));
          break;
        case(0b0011): //ROL
          result = (valA >> (valB % 16)) | (valA << (16 - (valB % 16)));
          break;
        default: //Unsupported Type
          printf("[EXECUTE] Unknown type.\n");
      }
      break;
    case 0b1001: //LW
      // Calculate the memory offset that will be added to DATA_SPACE (500) in memory stage
      // For "LW Rd, [Ra + imm]" - we want to calculate Ra + imm exactly
      result = valA + imm;
      
      // Make sure we're using the full 16-bit value
      printf("[EXECUTE_LW] R%u = MEM[R%u(%u) + %u] => offset=%u (will access address %u)\n", 
            regD, regA, valA, imm, result, result + DATA_SPACE);
      break;
    case 0b1010: //SW
      // Calculate the memory offset that will be added to DATA_SPACE (500) in memory stage
      // For "SW [Ra + imm], Rd" - we want to calculate Ra + imm exactly
      result = valA + imm;
      
      // Make sure we're using the full 16-bit value
      printf("[EXECUTE_SW] MEM[R%u(%u) + %u] = R%u(%u) => offset=%u (will access address %u)\n", 
            regA, valA, imm, regD, registers->R[regD], result, result + DATA_SPACE);
      break;
    case 0b1011: //BEQ
      result = registers->R[regD] == valA ? PC + imm : PC + 1;
      break;
    case 0b1111: //BLT
      result = registers->R[regD] < valA ? PC + imm : PC + 1;
      break;
    default: //NOOP, probably shouldn't happen at this stage
      printf("[EXECUTE] Unknown opcode: %u\n", opcode);
  }
  
  // Report pipeline state for UI - always report with the instruction text
  // we generated at the beginning
  printf("[PIPELINE]EXECUTE:%s:%d\n", instruction_text, PC);
  
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