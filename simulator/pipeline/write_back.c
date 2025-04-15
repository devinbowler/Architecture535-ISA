/* write_back.c */
#include "write_back.h"
#include "../pipeline.h"
#include "../memory.h"

extern REGISTERS *registers;

void write_back(PipelineState *pipeline) {
    if (!pipeline->MEM_WB.valid) {
        pipeline->MEM_WB_next.valid = false;
        printf("[PIPELINE]WRITEBACK:NOP:%d\n", pipeline->MEM_WB.pc);
        return;
    }

    // For memory and ALU operations, we update registers accordingly.
    uint16_t opcode = pipeline->MEM_WB.opcode;
    uint16_t regD = pipeline->MEM_WB.regD;
    uint16_t result = pipeline->MEM_WB.res;
    char instruction_text[50];

    switch (opcode) {
        case 0: // ADD
            sprintf(instruction_text, "ADD R%d = %d", regD, result);
            registers->R[regD] = result;
            break;
        case 1: // SUB
            sprintf(instruction_text, "SUB R%d = %d", regD, result);
            registers->R[regD] = result;
            break;
        case 2: // NAND
            sprintf(instruction_text, "NAND R%d = %d", regD, result);
            registers->R[regD] = result;
            break;
        case 3: // LUI
            sprintf(instruction_text, "LUI R%d = %d", regD, result);
            registers->R[regD] = result;
            break;
        case 4: // SW: nothing to update in registers.
            sprintf(instruction_text, "SW (no reg update)");
            break;
        case 5: // LW
        case 9:
            sprintf(instruction_text, "LW R%d = %d", regD, result);
            registers->R[regD] = result;
            break;
        case 11: // BEQ – typically no register update.
            sprintf(instruction_text, "BEQ (branch)");
            break;
        case 7: // JALR – update might have been done in execute.
            sprintf(instruction_text, "JALR (branch)");
            break;
        default:
            sprintf(instruction_text, "NOP");
            pipeline->MEM_WB_next.valid = false;
            break;
    }

    printf("[PIPELINE]WRITEBACK:%s:%d\n", instruction_text, pipeline->MEM_WB.pc);
    fflush(stdout);
}