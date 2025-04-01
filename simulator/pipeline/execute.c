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
  if(!execute_ready(pipeline)) return;
  uint16_t PC = pipeline->ID_EX.pc;
  uint16_t regD = registers[pipeline->ID_EX.regD];
  uint16_t regA = registers[pipeline->ID_EX.regA];
  uint16_t regB = registers[pipeline->ID_EX.regB];
  uint16_t imm = pipeline->ID_EX.imm;
  uint16_t opcode = pipeline->ID_EX.opcode;
  uint16_t result = 0;
  pipeline->EX_MEM_next.valid = true;
  
  switch(opcode) {
    case 0b0000: //ADD
      result = regA + regB;
      break;
    case 0b0001: //SUB
      result = regA - regB;
      break;
    case 0b0010: //AND
      result = regA & regB;
      break;
    case 0b0011: //OR
      result = regA | regB;
      break;
    case 0b0100: //XOR
      result = regA ^ regB;
      break;
    case 0b0101: //DIVMOD
      result = regA / regB;
      pipeline->EX_MEM_next.resMod = regA % regB;
      break;
    case 0b0110: //MUL
      result = regA * regB;
      break;
    case 0b0111: //CMP
      result = regA - regB;
      break;
    case 0b1000: //SHIFT
      switch(pipeline->ID_EX.type) {
        case(0b0000): //LSL
          result = regD << regA;
          break;
        case(0b0001): //LSR
          result = regD >> regA;
          break;
        case(0b0010): //ROR
          result = (regA << (regB % 16)) | (regA >> (16 - (regB % 16)));
          break;
        case(0b0011): //ROL
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
      result = regD == regA ? PC + imm : PC + 1;
      break;
    case 0b1111: //BLT
      result = regD < regA ? PC + imm : PC + 1;
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