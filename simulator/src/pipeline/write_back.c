// pipeline/write_back.c

#include <stdio.h>
#include <stdint.h>
#include "write_back.h"
#include "pipeline.h"
#include "memory.h"

extern REGISTERS *registers;

void write_back(PipelineState *pipeline) {
    // If there’s no valid entry from MEM/WB, bubble
    if (!pipeline->MEM_WB.valid) {
        pipeline->MEM_WB_next.valid = false;
        printf("[PIPELINE]WRITEBACK:NOP:%d\n", pipeline->MEM_WB.pc);
        return;
    }

    // Otherwise, we’ll write results back and pass through the fields
    uint16_t opcode = pipeline->MEM_WB.opcode;
    uint16_t regD   = pipeline->MEM_WB.regD;
    uint16_t result = pipeline->MEM_WB.res;
    char instruction_text[64];

    // Prepare next-stage state
    pipeline->WB_next.valid  = true;
    pipeline->WB_next.pc     = pipeline->MEM_WB.pc;
    pipeline->WB_next.opcode = opcode;
    pipeline->WB_next.regD   = regD;
    pipeline->WB_next.res    = result;

    switch (opcode) {
        case 0:  // ADD
            registers->R[regD] = result;
            sprintf(instruction_text, "ADD   R%u = %u", regD, result);
            break;
        case 1:  // SUB
            registers->R[regD] = result;
            sprintf(instruction_text, "SUB   R%u = %u", regD, result);
            break;
        case 2:  // NAND
            registers->R[regD] = result;
            sprintf(instruction_text, "NAND  R%u = %u", regD, result);
            break;
        case 3:  // LUI
            registers->R[regD] = result;
            sprintf(instruction_text, "LUI   R%u = %u", regD, result);
            break;
        case 4:  // SW: no register write
            sprintf(instruction_text, "SW    (no reg)");
            break;
        case 5:  // DIVMOD (we wrote quotient in res, remainder in resMod earlier)
            registers->R[regD] = result;
            sprintf(instruction_text, "DIVMOD R%u = %u", regD, result);
            break;
        case 6:  // MUL
            registers->R[regD] = result;
            sprintf(instruction_text, "MUL   R%u = %u", regD, result);
            break;
        case 7:  // CMP (we put result in SR / R14)
            registers->R[14] = result;
            sprintf(instruction_text, "CMP   SR = %u", result);
            break;
        case 8:  // Shifts: res already contains the shifted value
            registers->R[regD] = result;
            sprintf(instruction_text, "SH    R%u = %u", regD, result);
            break;
        case 9:  // LW (new opcode)
            registers->R[regD] = result;
            sprintf(instruction_text, "LW    R%u = %u", regD, result);
            break;
        case 10: // SW (new opcode)
            sprintf(instruction_text, "SW    (no reg)");
            break;
        case 11: // BEQ
            sprintf(instruction_text, "BEQ   (branch)");
            break;
        case 0xF:// BLT
            sprintf(instruction_text, "BLT   (branch)");
            break;
        default:
            // Unexpected opcode: treat as NOP
            pipeline->WB_next.valid = false;
            sprintf(instruction_text, "NOP");
            break;
    }

    // Final UI print
    printf("[PIPELINE]WRITEBACK:%s:%d\n", instruction_text, pipeline->MEM_WB.pc);
    fflush(stdout);
}