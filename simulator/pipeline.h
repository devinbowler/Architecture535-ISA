#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdint.h>
#include <stdbool.h>

// Pipeline registers

typedef struct {
    bool     valid; // Is the instruction a bubble or no.
    uint16_t pc;
    uint16_t instruction;
} IF_ID_Register;

typedef struct {
    bool     valid; // Is the instruction a bubble or no.
    uint16_t pc;
    uint16_t regD;
    uint16_t regA;
    uint16_t regB;
    uint16_t imm;
} ID_EX_Register;

typedef struct {
    IF_ID_Register IF_ID;
    ID_EX_Register ID_EX;

    // Registers for next cycles values.
    IF_ID_Register IF_ID_next;
    ID_EX_Register ID_EX_next;
} PipelineState;

extern PipelineState pipeline;

// Check if all stages are ready to proceed
bool pipeline_ready(PipelineState* pipeline);
void pipeline_step(PipelineState* pipeline, uint16_t* value);

#endif