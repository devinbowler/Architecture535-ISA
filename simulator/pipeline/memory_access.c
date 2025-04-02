#include "memory_access.h"
extern DRAM dram;
extern Cache *cache;
extern REGISTERS *registers;

// Function declaration
bool memory_access_ready(PipelineState *pipeline);

/**
 * @brief Implements the memory access stage of the pipeline. This will just use our existing cache and DRAM functions
 * @param pipeline the pipeline
 */
void memory_access(PipelineState *pipeline) {
    // Get pipeline state values
    uint16_t opcode = pipeline->EX_MEM.opcode;
    uint16_t regD = pipeline->EX_MEM.regD;
    uint16_t regA = pipeline->EX_MEM.regA;
    uint16_t regB = pipeline->EX_MEM.regB;
    uint16_t address = pipeline->EX_MEM.res; // Address calculated in execute stage
    uint16_t resMod = pipeline->EX_MEM.resMod;
    
    // Setup next pipeline stage
    pipeline->MEM_WB_next.valid = true;
    pipeline->MEM_WB_next.regD = regD;
    pipeline->MEM_WB_next.res = address; // Default to passing the address to writeback
    pipeline->MEM_WB_next.resMod = resMod;
    pipeline->MEM_WB_next.opcode = opcode; // Pass opcode to write back stage
    
    // Print memory access info
    printf("[MEMORY] opcode=%u rd=%u ra=%u rb=%u address=%u\n", 
           opcode, regD, regA, regB, address);
    fflush(stdout);

    if (opcode == 0b1001) { // LW - Load Word
        // Calculate effective address in data space (500-999)
        uint16_t mem_address = address + DATA_SPACE;
        
        printf("[MEM_ACCESS] LW: Accessing data address %u (DATA_SPACE %u + offset %u)\n", 
               mem_address, DATA_SPACE, address);
        
        // Read from memory and debug deeply
        uint16_t loaded_value = readFromMemory(&dram, mem_address);
        
        // Set the result to be written to regD in write-back stage
        pipeline->MEM_WB_next.res = loaded_value;
        
        printf("[MEM_LOAD] Reading R%u = MEM[%u] = %u\n", 
               regD, mem_address, loaded_value);
               
        // Explicitly report the memory read for UI
        printf("[MEM]%d:%d\n", mem_address, loaded_value);
    }
    else if (opcode == 0b1010) { // SW - Store Word
        // Calculate effective address in data space (500-999)
        uint16_t mem_address = address + DATA_SPACE;
        
        printf("[MEM_ACCESS] SW: Accessing data address %u (DATA_SPACE %u + offset %u)\n", 
               mem_address, DATA_SPACE, address);
        
        // Get the value to store from regD and store it
        uint16_t value_to_store = registers->R[regD];
        writeToMemory(&dram, mem_address, value_to_store);
        
        printf("[MEM_STORE] Writing MEM[%u] = R%u(%u)\n", 
               mem_address, regD, value_to_store);
        
        // Explicitly report memory update for UI - CRITICAL for the memory tab
        printf("[MEM]%d:%d\n", mem_address, value_to_store);
    }
    
    // For ALU operations, just pass through the result from execute
    if (opcode >= 0b0000 && opcode <= 0b0111) {
        printf("[MEM_PASS] Passing ALU result %u for R%u\n", address, regD);
    }
}

/**
 * @brief 
 * @param pipeline the pipeline
 * @return true if the memory stage is ready
 * @return false if the memory stage is not ready
 */
bool memory_access_ready(PipelineState *pipeline) {
  return pipeline->MEM_WB.valid;
}