#include "execute.h"
#include <stdint.h>

/**
 * @brief Implements the execute stage of the pipeline. This includes ALU operations and address calculations
 * @param pipeline the pipeline
 */
void execute(PipelineState *pipeline) {
  if(!execute_ready(pipeline)) return;
  pipeline->EX_MEM_next.valid = true;
  uint16_t PC = pipeline->ID_EX.pc;
  switch(pipeline->ID_EX.opcode) {
    case 0b0000:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA + pipeline->ID_EX.regB;
      break;
    case 0b0001:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA - pipeline->ID_EX.regB;
      break;
    case 0b0010:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA & pipeline->ID_EX.regB;
      break;
    case 0b0011:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA | pipeline->ID_EX.regB;
      break;
    case 0b0100:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA ^ pipeline->ID_EX.regB;
      break;
    case 0b0101:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA / pipeline->ID_EX.regB;
      pipeline->EX_MEM_next.resMod = pipeline->ID_EX.regA % pipeline->ID_EX.regB;
      break;
    case 0b0110:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA * pipeline->ID_EX.regB;
      break;
    case 0b0111:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA - pipeline->ID_EX.regB;
      break;
    case 0b1000:
      break;
    case 0b1001:

      break;
    case 0b1010:
      break;
    case 0b1011:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regD == pipeline->ID_EX.regA ? PC + pipeline->ID_EX.imm : PC + 1;
      break;
    case 0b1111:
      PC = pipeline->ID_EX.pc;
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regD < pipeline->ID_EX.regA ? PC + pipeline->ID_EX.imm : PC + 1;
      break;
  }
}

bool execute_ready(PipelineState *pipeline) {
    return pipeline->EX_MEM.valid; 
}
