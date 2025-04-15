// execute.c
#include <stdio.h>
#include <stdlib.h>
#include "execute.h"
#include "../pipeline.h"
#include "../memory.h"

// Externally declared register file pointer.
extern REGISTERS *registers;
bool branch_taken = false;
uint16_t branch_target_address = 0;

void flush_pipeline(PipelineState *pipeline) {
    // Invalidate all next-stage registers.
    pipeline->IF_ID_next.valid  = false;
    pipeline->ID_EX_next.valid  = false;
    pipeline->EX_MEM_next.valid = false;
    pipeline->MEM_WB_next.valid = false;
    pipeline->WB_next.valid     = false;
    printf("[PIPELINE] Branch detected: Flushing pipeline\n");
}

void execute(PipelineState *pipeline) {
    uint16_t pc = pipeline->ID_EX.pc;
    if (!pipeline->ID_EX.valid) {
        pipeline->EX_MEM_next.valid = false;
        printf("[PIPELINE]EXECUTE:NOP:%d\n", pc);
        fflush(stdout);
        return;
    }
    
    pipeline->EX_MEM_next.valid = true;
    uint16_t opcode = pipeline->ID_EX.opcode;
    uint16_t regD = pipeline->ID_EX.regD;
    uint16_t regA = pipeline->ID_EX.regA;
    uint16_t regB = pipeline->ID_EX.regB;
    uint16_t imm = pipeline->ID_EX.imm;
    uint16_t result = 0;

    uint16_t valA = registers->R[regA];
    uint16_t valB = registers->R[regB];

    pipeline->EX_MEM_next.regD = regD;
    pipeline->EX_MEM_next.regA = regA;
    pipeline->EX_MEM_next.regB = regB;
    pipeline->EX_MEM_next.opcode = opcode;
    pipeline->EX_MEM_next.pc = pc;

    char instruction_text[50];
    switch (opcode) {
        case 0: // ADD
            result = valA + valB;
            sprintf(instruction_text, "ADD R%d, R%d, R%d", regD, regA, regB);
            printf("[EXECUTE_ADD] R%d = %u + %u = %u\n", regD, valA, valB, result);
            break;
        case 1: // SUB
            result = valA - valB;
            sprintf(instruction_text, "SUB R%d, R%d, R%d", regD, regA, regB);
            printf("[EXECUTE_SUB] R%d = %u - %u = %u\n", regD, valA, valB, result);
            break;
        case 2: // NAND
            result = ~(valA & valB);
            sprintf(instruction_text, "NAND R%d, R%d, R%d", regD, regA, regB);
            printf("[EXECUTE_NAND] R%d = ~( %u & %u ) = %u\n", regD, valA, valB, result);
            break;
        case 3: // LUI
            result = imm << 12;
            sprintf(instruction_text, "LUI R%d, %d", regD, imm);
            printf("[EXECUTE_LUI] R%d = %u << 12 = %u\n", regD, imm, result);
            break;
        case 4: // SW (old or new)
        case 10:
            result = valA + imm;
            sprintf(instruction_text, "SW [R%d+%d], R%d", regA, imm, regD);
            printf("[EXECUTE_SW] Calculated address = %u + %u = %u\n", valA, imm, result);
            break;
        case 5: // LW (old or new)
        case 9:
            result = valA + imm;
            sprintf(instruction_text, "LW R%d, [R%d+%d]", regD, regA, imm);
            printf("[EXECUTE_LW] Calculated address = %u + %u = %u\n", valA, imm, result);
            break;
        case 11: // BEQ
            if (valA == registers->R[regD]) {
                result = pc + imm;
                branch_taken = true;
                branch_target_address = result;
                flush_pipeline(pipeline);
                printf("[EXECUTE_BEQ] Branch taken: %u == R%d, setting PC = %u\n", valA, regD, result);
            } else {
                result = pc + 1;
                printf("[EXECUTE_BEQ] Branch not taken: %u != R%d\n", valA, regD);
            }
            sprintf(instruction_text, "BEQ R%d, R%d, %d", regD, regA, imm);
            break;
        case 7: // JALR
            result = pc + 1;
            pipeline->EX_MEM_next.resMod = valA;
            branch_taken = true;
            branch_target_address = valA;
            flush_pipeline(pipeline);
            sprintf(instruction_text, "JALR R%d, R%d", regD, regA);
            printf("[EXECUTE_JALR] Setting PC=R%d (%u), result=%u\n", regA, valA, result);
            break;
        default:
            printf("[EXECUTE] Unknown opcode: %u\n", opcode);
            sprintf(instruction_text, "NOP");
            pipeline->EX_MEM_next.valid = false;
            break;
    }
    
    printf("[PIPELINE]EXECUTE:%s:%d\n", instruction_text, pc);
    pipeline->EX_MEM_next.res = result;
    fflush(stdout);
}

bool execute_ready(PipelineState *pipeline) {
    return pipeline->EX_MEM.valid;
}
