#include <stdio.h>
#include <stdlib.h>
#include "execute.h"
#include "../pipeline.h"
#include "../memory.h"

extern REGISTERS *registers;

/**
 * @brief Implements the execute stage of the pipeline. This includes ALU operations and address calculations
 * @param pipeline the pipeline
 */
void execute(PipelineState *pipeline) {
  if(!execute_ready(pipeline)) return;
  uint16_t PC = registers->R[pipeline->ID_EX.pc];
  uint16_t regD = registers->R[pipeline->ID_EX.regD];
  uint16_t regA = registers->R[pipeline->ID_EX.regA];
  uint16_t regB = registers->R[pipeline->ID_EX.regB];
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
      registers->R[14] = regA - regB;
      break;
    case 0b1000: //SHIFT
      switch(pipeline->ID_EX.type) {
        case 0b0000: //LSL
          result = regD << regA;
          break;
        case 0b0001: //LSR
          result = regD >> regA;
          break;
        case 0b0010: //ROR
          result = (regA << (regB % 16)) | (regA >> (16 - (regB % 16)));
          break;
        case 0b0011: //ROL
          result = (regA >> (regB % 16)) | (regA << (16 - (regB % 16)));
          break;
        default: //Unsupported Type
          printf("[EXECUTE] Unknown type.\n");
          while(true) {}
      }
      break;
    case 0b1001: //LW
      result = pipeline->ID_EX.regA + imm;
      break;
    case 0b1010: //SW
      result = pipeline->ID_EX.regA + imm; 
      break;
    case 0b1011: //BEQ
      registers->R[15] = regD == regA ? PC + imm : PC + 1;
      break;
    case 0b1111: //BLT
      registers->R[15] = regD < regA ? PC + imm : PC + 1;
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
