#include "hazards.h"
#include <stdio.h>

// Global variable to track if pipeline is stalled due to data hazard
bool data_hazard_stall = false;
uint16_t stall_cycles_remaining = 0;

/**
 * @brief Detects data hazards between instructions in different pipeline stages
 *
 * This function checks for RAW hazards where an instruction in ID/EX
 * needs a register value that is being produced by an instruction in
 * EX/MEM or MEM/WB stage
 *
 * @param pipeline the pipeline
 * @return the hazard results
 */
HazardInfo detect_hazards(PipelineState *pipeline) {
    HazardInfo hazard = {false, false, 0, 0, 0, 0, 0};
    // If ID/EX stage isn't valid, no hazard to detect
    if (!pipeline->ID_EX.valid) {
        return hazard;
    }
    // Get source registers for the instruction in ID/EX
    uint16_t regA = pipeline->ID_EX.regA;
    uint16_t regB = pipeline->ID_EX.regB;

    // For each possible source register, check for hazards
    // RAW Hazards from instruction in EX/MEM
    if (pipeline->EX_MEM.valid) {
        uint16_t ex_mem_dest = pipeline->EX_MEM.regD;
        uint16_t ex_mem_opcode = pipeline->EX_MEM.opcode;
        
        // Only consider instructions that write to registers
        bool ex_mem_writes_reg = (ex_mem_opcode <= 6) || // ALU ops
                                 (ex_mem_opcode == 9); // LW 
        
        // Check if regA depends on previous result
        if (ex_mem_writes_reg && regA == ex_mem_dest && regA != 0) {
            hazard.detected = true;
            hazard.source_reg = ex_mem_dest;
            hazard.target_reg = regA;
            hazard.source_stage = 1; // EX/MEM stage
            
            // LW (Load Word) needs an extra cycle - can't forward directly from EX/MEM
            if (ex_mem_opcode == 9) {
                hazard.requires_stall = true;
                hazard.stall_cycles = 1;
                printf("[HAZARD] Load-use hazard detected: R%u depends on LW result\n", regA);
            } else {
                // Can forward the result
                hazard.forwarded_value = pipeline->EX_MEM.res;
                printf("[HAZARD] RAW hazard detected: R%u depends on EX/MEM result, forwarding value %u\n", 
                       regA, hazard.forwarded_value);
            }
            return hazard;
        }
        
        // Check if regB depends on previous result
        if (ex_mem_writes_reg && regB == ex_mem_dest && regB != 0) { // Only for ops that use regB as source
            hazard.detected = true;
            hazard.source_reg = ex_mem_dest;
            hazard.target_reg = regB;
            hazard.source_stage = 1; // EX/MEM stage
            
            // LW (Load Word) needs an extra cycle
            if (ex_mem_opcode == 9) {
                hazard.requires_stall = true;
                hazard.stall_cycles = 1;
                printf("[HAZARD] Load-use hazard detected: R%u depends on LW result\n", regB);
            } else {
                // Can forward the result
                hazard.forwarded_value = pipeline->EX_MEM.res;
                printf("[HAZARD] RAW hazard detected: R%u depends on EX/MEM result, forwarding value %u\n", 
                       regB, hazard.forwarded_value);
            }
            return hazard;
        }
    }
    
    // RAW Hazards from instruction in MEM/WB
    if (pipeline->MEM_WB.valid) {
        uint16_t mem_wb_dest = pipeline->MEM_WB.regD;
        uint16_t mem_wb_opcode = pipeline->MEM_WB.opcode;
        
        // Only consider instructions that write to registers
        bool mem_wb_writes_reg = (mem_wb_opcode <= 6) || // ALU ops
                                 (mem_wb_opcode == 9); // LW
        
        // Check if regA depends on previous result
        if (mem_wb_writes_reg && regA == mem_wb_dest && regA != 0) {
            hazard.detected = true;
            hazard.source_reg = mem_wb_dest;
            hazard.target_reg = regA;
            hazard.source_stage = 2; // MEM/WB stage
            hazard.forwarded_value = pipeline->MEM_WB.res;
            printf("[HAZARD] RAW hazard detected: R%u depends on MEM/WB result, forwarding value %u\n", 
                   regA, hazard.forwarded_value);
            return hazard;
        }
        
        // Check if regB depends on previous result
        if (mem_wb_writes_reg && regB == mem_wb_dest && regB != 0) { // Only for ops that use regB as source
            hazard.detected = true;
            hazard.source_reg = mem_wb_dest;
            hazard.target_reg = regB;
            hazard.source_stage = 2; // MEM/WB stage
            hazard.forwarded_value = pipeline->MEM_WB.res;
            printf("[HAZARD] RAW hazard detected: R%u depends on MEM/WB result, forwarding value %u\n", 
                   regB, hazard.forwarded_value);
            return hazard;
        }
    }
    
    return hazard;
}

/**
 * @brief Resolves detected data hazards by forwarding values or stalling the pipeline
 * @param pipeline the pipeline
 * @param hazard the hazard results in the pipeline
 */
void resolve_hazards(PipelineState *pipeline, HazardInfo *hazard) {
    if (!hazard->detected) {
        return;
    }
    
    // If stall is required, set up stalling state
    if (hazard->requires_stall) {
        data_hazard_stall = true;
        stall_cycles_remaining = hazard->stall_cycles;
        printf("[HAZARD] Stalling pipeline for %u cycles\n", stall_cycles_remaining);
    } else {
        // Forward result
        forward_result(pipeline, hazard);
    }
}

/**
 * @brief Forwards the result from a later pipeline stage to the execute stage
 * @param pipeline the pipeline
 * @param hazard the hazards in the pipeline
 */
void forward_result(PipelineState *pipeline, HazardInfo *hazard) {
    extern REGISTERS *registers;
    
    // Check which register needs the forwarded value
    uint16_t regA = pipeline->ID_EX.regA;
    uint16_t regB = pipeline->ID_EX.regB;
    uint16_t opcode = pipeline->ID_EX.opcode;
    
    // For debugging, store original values before forwarding
    uint16_t orig_valA = registers->R[regA];
    uint16_t orig_valB = opcode <= 2 || opcode == 11 ? registers->R[regB] : 0;
    
    // Forward to the appropriate register
    if (hazard->target_reg == regA) {
        registers->R[regA] = hazard->forwarded_value;
        printf("[FORWARD] R%u updated from %u to %u for execution\n", 
               regA, orig_valA, hazard->forwarded_value);
    } else if (hazard->target_reg == regB) {
        registers->R[regB] = hazard->forwarded_value;
        printf("[FORWARD] R%u updated from %u to %u for execution\n", 
               regB, orig_valB, hazard->forwarded_value);
    }
}
