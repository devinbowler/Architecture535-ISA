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
  // if(!execute_ready(pipeline)) return;
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
  
  // Print execute info with actual register values
  printf("[EXECUTE] opcode=%u rd=%u ra=%u(val=%u) rb=%u(val=%u)\n", 
         opcode, regD, regA, valA, regB, valB);
  fflush(stdout);
  
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
          while(true) {}
      }
      break;
    case 0b1001: //LW
      // Calculate the memory offset (will be added to DATA_SPACE in memory stage)
      // If we want R0 + 15 to access address 515, then we calculate:
      // offset = R0 + 15 = 0 + 15 = 15
      // Final address in memory stage will be DATA_SPACE + offset = 500 + 15 = 515
      result = valA + imm;
      printf("[EXECUTE_LW] R%u = MEM[R%u(%u) + %u] => Load from offset %u, will access address %u\n", 
            regD, regA, valA, imm, result, result + DATA_SPACE);
      break;
    case 0b1010: //SW
      // Calculate the memory offset (will be added to DATA_SPACE in memory stage)
      // If we want R0 + 15 to access address 515, then we calculate:
      // offset = R0 + 15 = 0 + 15 = 15
      // Final address in memory stage will be DATA_SPACE + offset = 500 + 15 = 515
      result = valA + imm; 
      printf("[EXECUTE_SW] MEM[R%u(%u) + %u] = R%u(%u) => Store at offset %u, will access address %u\n", 
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
      while(true) {}
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