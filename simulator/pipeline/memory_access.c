#include "memory_access.h"
extern DRAM dram;
extern Cache *cache;
extern REGISTERS *registers;

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
        // Calculate effective address
        uint16_t mem_address = address;
        if (address < DATA_SPACE) {
            mem_address += DATA_SPACE;
        }
        
        // Use cache for reading
        uint16_t loaded_value;
        if (cache != NULL) {
            printf("[LOG] Attempting cache read from address %u\n", mem_address);
            loaded_value = read_cache(cache, &dram, mem_address);
            printf("[LOG] Cache read result: %u\n", loaded_value);
        } else {
            loaded_value = readFromMemory(&dram, mem_address);
        }
        
        // Set the result to be written to regD in write-back stage
        pipeline->MEM_WB_next.res = loaded_value;
        
        printf("[MEM_LOAD] Reading from address %u, value %u, to register R%u\n", 
               mem_address, loaded_value, regD);
               
        // Report memory read to UI
        printf("[MEM]%d:%d\n", mem_address, loaded_value);
    }
    else if (opcode == 0b1010) { // SW - Store Word
        // Calculate effective address
        uint16_t mem_address = address;
        if (address < DATA_SPACE) {
            mem_address += DATA_SPACE;
        }
        
        // Get the value to store from regD
        uint16_t value_to_store = registers->R[regD];
        
        // Use cache for writing
        if (cache != NULL) {
            printf("[LOG] Attempting cache write to address %u with value %u\n", 
                   mem_address, value_to_store);
            write_through(cache, &dram, mem_address, value_to_store);
            printf("[LOG] Cache write complete\n");
        } else {
            writeToMemory(&dram, mem_address, value_to_store);
        }
        
        printf("[MEM_STORE] Writing to address %u, value %u, from register R%u\n", 
               mem_address, value_to_store, regD);
        
        // Report memory update to UI
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
bool memory_ready(PipelineState *pipeline) {
  return pipeline->MEM_WB.valid;
}