#include "write_back.h"
#include "pipeline.h"
#include "../memory.h"

extern REGISTERS *registers;

/**
 * @brief Write-back stage.
 *
 * Processes the instruction from MEM_WB and updates the register file if needed.
 * Only prints the minimal messages (which the API uses) to stdout.
 */
void write_back(PipelineState *pipeline) {
    // If there is no valid instruction in MEM_WB, then propagate a bubble.
    if (!pipeline->MEM_WB.valid) {
        pipeline->MEM_WB_next.valid = false;
        return;
    }

    // Mark the next cycle's WB as valid.
    pipeline->MEM_WB_next.valid = true;

    uint16_t opcode = pipeline->MEM_WB.opcode;
    uint16_t regD = pipeline->MEM_WB.regD;
    uint16_t regB = pipeline->MEM_WB.regB;
    uint16_t result = pipeline->MEM_WB.res;
    uint16_t resMod = pipeline->MEM_WB.resMod;
    char instruction_text[50] = "NOP";

    // For ALU operations (opcodes 0 to 7) and SHIFT (opcode 8)
    if (opcode <= 0b0111) {
        switch(opcode) {
            case 0b0000: sprintf(instruction_text, "ADD R%d = %d", regD, result); break;
            case 0b0001: sprintf(instruction_text, "SUB R%d = %d", regD, result); break;
            case 0b0010: sprintf(instruction_text, "AND R%d = %d", regD, result); break;
            case 0b0011: sprintf(instruction_text, "OR R%d = %d", regD, result); break;
            case 0b0100: sprintf(instruction_text, "XOR R%d = %d", regD, result); break;
            case 0b0101: sprintf(instruction_text, "DIVMOD R%d = %d", regD, result); break;
            case 0b0110: sprintf(instruction_text, "MUL R%d = %d", regD, result); break;
            case 0b0111: sprintf(instruction_text, "CMP R%d = %d", regD, result); break;
            default: sprintf(instruction_text, "UNKNOWN ALU op=%d", opcode);
        }
        registers->R[regD] = result;
        printf("[WB] %s\n", instruction_text);
        printf("[REG]%d:%d\n", regD, result);

        if (opcode == 0b0101) {  // DIVMOD: update an additional register.
            registers->R[regB] = resMod;
            printf("[WB] DIVMOD Extra: R%d = %d\n", regB, resMod);
            printf("[REG]%d:%d\n", regB, resMod);
        }
    }
    else if (opcode == 0b1000) { 
        // SHIFT operations
        sprintf(instruction_text, "SHIFT R%d = %d", regD, result);
        registers->R[regD] = result;
        printf("[WB] %s\n", instruction_text);
        printf("[REG]%d:%d\n", regD, result);
    }
    else if (opcode == 0b1001 || opcode == 5) {
        // LW instruction: load value from memory.
        sprintf(instruction_text, "LW R%d = %d", regD, result);
        registers->R[regD] = result;
        printf("[WB] %s\n", instruction_text);
        printf("[REG]%d:%d\n", regD, result);
    }
    else if (opcode == 0b1010 || opcode == 4) {
        // SW: store word; no register update here.
        sprintf(instruction_text, "SW [R%d + %d]", pipeline->MEM_WB.regA, pipeline->MEM_WB.imm);
    }
    else if (opcode == 0b1011) {
        sprintf(instruction_text, "BEQ R%d, R%d, %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.imm);
    }
    else if (opcode == 0b1111) {
        sprintf(instruction_text, "BLT R%d, R%d, %d", regD, pipeline->MEM_WB.regA, pipeline->MEM_WB.imm);
    }
    else {
        sprintf(instruction_text, "UNKNOWN op=%d", opcode);
    }

    // Print a concise pipeline summary for this stage.
    if (!pipeline->MEM_WB.valid || opcode == 0) {
        printf("[PIPELINE]WRITEBACK:Bubble:%d\n", pipeline->MEM_WB.pc);
    } else {
        printf("[PIPELINE]WRITEBACK:%s:%d\n", instruction_text, pipeline->MEM_WB.pc);
    }
    
    fflush(stdout);
}