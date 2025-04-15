// execute.c
#include <stdio.h>
#include <stdlib.h>
#include "execute.h"
#include "../pipeline.h"
#include "../memory.h"

// Externally declared register file pointer.
extern REGISTERS *registers;

// Global branch status variables.
bool branch_taken = false;
uint16_t branch_target_address = 0;

// Flush the pipeline: invalidate next‐cycle registers in the earlier stages.
void flush_pipeline(PipelineState *pipeline) {
    pipeline->IF_ID_next.valid  = false;
    pipeline->ID_EX_next.valid  = false;
    pipeline->EX_MEM_next.valid = false;
    pipeline->MEM_WB_next.valid = false;
    pipeline->WB_next.valid     = false;
    printf("[PIPELINE] Branch detected: Flushing pipeline\n");
}

// The execute stage: compute ALU results, compare values for CMP, and handle branches.
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
        case 0x0: // ADD
            result = valA + valB;
            sprintf(instruction_text, "ADD R%d, R%d, R%d", regD, regA, regB);
            printf("[EXECUTE_ADD] R%d = %u + %u = %u\n", regD, valA, valB, result);
            break;
        case 0x1: // SUB
            result = valA - valB;
            sprintf(instruction_text, "SUB R%d, R%d, R%d", regD, regA, regB);
            printf("[EXECUTE_SUB] R%d = %u - %u = %u\n", regD, valA, valB, result);
            break;
        case 0x2: // AND
            result = valA & valB;
            sprintf(instruction_text, "AND R%d, R%d, R%d", regD, regA, regB);
            printf("[EXECUTE_AND] R%d = %u & %u = %u\n", regD, valA, valB, result);
            break;
        case 0x3: // OR
            result = valA | valB;
            sprintf(instruction_text, "OR R%d, R%d, R%d", regD, regA, regB);
            printf("[EXECUTE_OR] R%d = %u | %u = %u\n", regD, valA, valB, result);
            break;
        case 0x4: // XOR
            result = valA ^ valB;
            sprintf(instruction_text, "XOR R%d, R%d, R%d", regD, regA, regB);
            printf("[EXECUTE_XOR] R%d = %u xor %u = %u\n", regD, valA, valB, result);
            break;
        case 0x5: // DIVMOD
            if (valB == 0) {
                result = 0;
                pipeline->EX_MEM_next.resMod = 0;
                sprintf(instruction_text, "DIVMOD R%d, R%d, R%d (Div0)", regD, regA, regB);
                printf("[EXECUTE_DIVMOD] Division by zero, setting result and remainder to 0\n");
            } else {
                result = valA / valB;
                pipeline->EX_MEM_next.resMod = valA % valB;
                sprintf(instruction_text, "DIVMOD R%d, R%d, R%d", regD, regA, regB);
                printf("[EXECUTE_DIVMOD] R%d = %u / %u = %u, remainder = %u\n", 
                       regD, valA, valB, result, pipeline->EX_MEM_next.resMod);
            }
            break;
        case 0x6: // MUL
            result = valA * valB;
            sprintf(instruction_text, "MUL R%d, R%d, R%d", regD, regA, regB);
            printf("[EXECUTE_MUL] R%d = %u * %u = %u\n", regD, valA, valB, result);
            break;
        case 0x7: // CMP
            // For CMP, we subtract and update the status register (assumed here to be R14).
            result = registers->R[regD] - valA;
            sprintf(instruction_text, "CMP R%d, R%d, R%d", regD, regA, regB);
            printf("[EXECUTE_CMP] R%d - R%d = %u\n", regD, regA, result);
            registers->R[14] = result;
            break;
        case 0x8: { // LSH/ROT (RTRI)
            // The RD register field (here, using regD as "type") select the shift type.
            uint16_t shift_type = regD;
            uint16_t operand = registers->R[regA];
            switch (shift_type) {
                case 0: // LSL
                    result = operand << imm;
                    sprintf(instruction_text, "LSL R%d, R%d, %d", regD, regA, imm);
                    printf("[EXECUTE_LSL] R%d = R%d << %d = %u\n", regD, regA, imm, result);
                    break;
                case 1: // LSR
                    result = operand >> imm;
                    sprintf(instruction_text, "LSR R%d, R%d, %d", regD, regA, imm);
                    printf("[EXECUTE_LSR] R%d = R%d >> %d = %u\n", regD, regA, imm, result);
                    break;
                case 2: // ROL (rotate left)
                {
                    result = (operand << imm) | (operand >> (16 - imm));
                    sprintf(instruction_text, "ROL R%d, R%d, %d", regD, regA, imm);
                    printf("[EXECUTE_ROL] R%d = R%d rotated left by %d = %u\n", regD, regA, imm, result);
                    break;
                }
                case 3: // ROR (rotate right)
                {
                    result = (operand >> imm) | (operand << (16 - imm));
                    sprintf(instruction_text, "ROR R%d, R%d, %d", regD, regA, imm);
                    printf("[EXECUTE_ROR] R%d = R%d rotated right by %d = %u\n", regD, regA, imm, result);
                    break;
                }
                default:
                    sprintf(instruction_text, "LSH Unknown");
                    result = 0;
                    break;
            }
            break;
        }
        // RRI-Type instructions:
        case 0x9: // LW
            // Compute effective address: R[regA] + imm.
            result = registers->R[regA] + imm;
            sprintf(instruction_text, "LW R%d, [R%d+%d]", regD, regA, imm);
            printf("[EXECUTE_LW] Effective address = %u + %u = %u\n", registers->R[regA], imm, result);
            break;
        case 0xA: // SW
            // Compute effective address: R[regA] + imm.
            result = registers->R[regA] + imm;
            sprintf(instruction_text, "SW [R%d+%d], R%d", regA, imm, regD);
            printf("[EXECUTE_SW] Effective address = %u + %u = %u\n", registers->R[regA], imm, result);
            break;
        case 0xB: // BEQ
            // Compare R[regD] and R[regA]; if equal, branch.
            if (registers->R[regD] == registers->R[regA]) {
                result = pc + imm;
                branch_taken = true;
                branch_target_address = result;
                flush_pipeline(pipeline);
                printf("[EXECUTE_BEQ] Branch taken: R%d (%u) == R%d (%u), new PC = %u\n",
                        regD, registers->R[regD], regA, registers->R[regA], result);
            } else {
                result = pc + 1;
                printf("[EXECUTE_BEQ] Branch not taken: R%d (%u) != R%d (%u)\n",
                        regD, registers->R[regD], regA, registers->R[regA]);
            }
            sprintf(instruction_text, "BEQ R%d, R%d, %d", regD, regA, imm);
            break;
        case 0xF: // BLT
            // Branch if R[regD] < R[regA]
            if (registers->R[regD] < registers->R[regA]) {
                result = pc + imm;
                branch_taken = true;
                branch_target_address = result;
                flush_pipeline(pipeline);
                printf("[EXECUTE_BLT] Branch taken: R%d (%u) < R%d (%u), new PC = %u\n",
                        regD, registers->R[regD], regA, registers->R[regA], result);
            } else {
                result = pc + 1;
                printf("[EXECUTE_BLT] Branch not taken: R%d (%u) >= R%d (%u)\n",
                        regD, registers->R[regD], regA, registers->R[regA]);
            }
            sprintf(instruction_text, "BLT R%d, R%d, %d", regD, regA, imm);
            break;
        default:
            printf("[EXECUTE] Unknown opcode: %u\n", opcode);
            pipeline->EX_MEM_next.valid = false;
            break;
    }

    printf("[PIPELINE]EXECUTE:%s:%d\n", instruction_text, pc);
    pipeline->EX_MEM_next.res = result;
    fflush(stdout);
}
