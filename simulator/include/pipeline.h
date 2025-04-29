#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdint.h>
#include <stdbool.h>

// Pipeline registers

typedef struct {
    bool     valid;       // Is the instruction a bubble or no.
    bool     squashed;    // Is the instruction squashed due to branch
    uint16_t pc;
    uint16_t instruction;
} IF_ID_Register;

typedef struct {
    bool     valid;       // Is the instruction a bubble or no.
    bool     squashed;    // Is the instruction squashed due to branch
    uint16_t pc;
    uint16_t regD;
    uint16_t regA;
    uint16_t regB;
    uint16_t imm;         // Make sure this is 16 bits for correct offset values
    uint16_t opcode;
    uint16_t type;
} ID_EX_Register;

typedef struct {
    bool     valid;
    bool     squashed;    // Is the instruction squashed due to branch
    uint16_t pc;
    uint16_t regD;
    uint16_t regA;
    uint16_t regB;
    uint16_t imm;
    uint16_t opcode;
    uint16_t res;
    uint16_t resMod;
    uint16_t functional_unit;  // Added for scoreboard tracking
} EX_MEM_Register;

typedef struct {
    bool     valid;
    bool     squashed;    // Is the instruction squashed due to branch
    uint16_t pc;
    uint16_t regD;
    uint16_t regA;
    uint16_t regB;
    uint16_t imm;
    uint16_t opcode;
    uint16_t res;
    uint16_t resMod;
    uint16_t functional_unit;  // Added for scoreboard tracking
} MEM_WB_Register;

typedef struct {
    bool     valid;
    bool     squashed;    // Is the instruction squashed due to branch
    uint16_t pc;
    uint16_t regD;
    uint16_t regA;
    uint16_t regB;
    uint16_t imm;
    uint16_t opcode;
    uint16_t res;
    uint16_t resMod;
} WB_Register;

typedef struct {
    IF_ID_Register IF_ID;
    ID_EX_Register ID_EX;
    EX_MEM_Register EX_MEM;
    MEM_WB_Register MEM_WB;
    WB_Register WB;
    // Registers for next cycles values.
    IF_ID_Register IF_ID_next;
    ID_EX_Register ID_EX_next;
    EX_MEM_Register EX_MEM_next;
    MEM_WB_Register MEM_WB_next;
    WB_Register WB_next;
} PipelineState;

extern PipelineState pipeline;

void pipeline_step(PipelineState* pipeline, uint16_t* value);
void mark_subsequent_instructions_as_squashed(PipelineState* pipeline);

#endif
