#include "execute.h"
#include <stdint.h>

/**
 * @brief Implements the execute stage of the pipeline. This includes ALU operations and address calculations
 * @param pipeline the pipeline
 */
void execute(PipelineState *pipeline) {
  switch(pipeline->ID_EX.op) {
    case 0000:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA + pipeline->ID_EX.regB;
      break;
    case 0001:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA - pipeline->ID_EX.regB;
      break;
    case 0010:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA & pipeline->ID_EX.regB;
      break;
    case 0011:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA | pipeline->ID_EX.regB;
      break;
    case 0100:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA ^ pipeline->ID_EX.regB;
      break;
    case 0101:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA / pipeline->ID_EX.regB;
      pipeline->EX_MEM_next.resMod = pipeline->ID_EX.regA % pipeline->ID_EX.regB;
      break;
    case 0110:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA * pipeline->ID_EX.regB;
      break;
    case 0111:
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regA - pipeline->ID_EX.regB;
      break;
    case 1000:
      break;
    case 1001:

      break;
    case 1010:
      break;
    case 1011:
      uint16_t PC = pipeline->ID_EX.pc;
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regD == pipeline->ID_EX.regA ? PC + pipeline->ID_EX.imm : PC + 1;
      break;
    case 1111:
      PC = pipeline->ID_EX.pc;
      pipeline->EX_MEM_next.res = pipeline->ID_EX.regD < pipeline->ID_EX.regA ? PC + pipeline->ID_EX.imm : PC + 1;
      break;
  }
}

bool execute_ready(PipelineState *pipeline) {
    return pipeline->EX_MEM.valid; 
}
