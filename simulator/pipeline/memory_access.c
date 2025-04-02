#include "memory_access.h"
extern DRAM dram;
extern Cache *cache;
extern REGISTERS *registers;

// Global flag to indicate when memory operation is in progress
bool memory_operation_in_progress = false;

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
    
    // Track memory access state for delays
    static bool memory_busy = false;
    static uint16_t memory_delay = 0;
    static uint16_t memory_target_delay = 0;
    static uint16_t pending_mem_address = 0;
    static uint16_t pending_opcode = 0;
    static uint16_t pending_regD = 0;
    static uint16_t pending_regA = 0;
    static uint16_t pending_value = 0;
    
    // Setup next pipeline stage
    pipeline->MEM_WB_next.valid = true;
    pipeline->MEM_WB_next.regD = regD;
    pipeline->MEM_WB_next.resMod = resMod;
    pipeline->MEM_WB_next.opcode = opcode; // Pass opcode to write back stage
    pipeline->MEM_WB_next.regA = regA;
    pipeline->MEM_WB_next.regB = regB;
    pipeline->MEM_WB_next.imm = pipeline->EX_MEM.imm;

    // For non-memory operations, pass the result directly
    if (opcode != 4 && opcode != 5 && opcode != 9 && opcode != 10) {
        pipeline->MEM_WB_next.res = pipeline->EX_MEM.res;
    }
    // For memory operations, res will be set during the memory access completion

    // Print memory access info
    printf("[MEMORY] opcode=%u rd=%u ra=%u rb=%u address=%u\n", 
           opcode, regD, regA, regB, address);
    
    // Generate stage information for pipeline visualization
    char instruction_text[50] = "NOP";

    // If we're currently waiting on a memory operation
    if (memory_busy) {
        memory_operation_in_progress = true; // Set global flag to freeze pipeline
        memory_delay++;
        printf("[MEM_DELAY] Cycle %u of %u\n", memory_delay, memory_target_delay);
        
        // If we've reached our delay, complete the memory operation
        if (memory_delay >= memory_target_delay) {
            printf("[MEM_COMPLETE] Memory operation completed after %u cycles\n", memory_delay);
            
            if (pending_opcode == 0b1001) { // LW - Load Word completed
                // Get the loaded value
                uint16_t loaded_value;
                if (memory_target_delay == 1) {
                    // Cache hit
                    printf("[MEM_CACHE_HIT] Reading from cache address %u\n", pending_mem_address);
                    loaded_value = read_cache(cache, &dram, pending_mem_address);
                } else {
                    // DRAM access
                    printf("[MEM_DRAM_ACCESS] Reading from DRAM address %u\n", pending_mem_address);
                    loaded_value = readFromMemory(&dram, pending_mem_address);
                }
                
                // Set the result to be written to regD in write-back stage
                pipeline->MEM_WB_next.res = loaded_value;
                
                printf("[MEM_LOAD_COMPLETE] Read from address %u, value %u, to register R%u\n", 
                       pending_mem_address, loaded_value, pending_regD);
                       
                // Report memory read to UI
                printf("[MEM]%d:%d\n", pending_mem_address, loaded_value);
                sprintf(instruction_text, "LW R%d, [R%d + %d]", pending_regD, pending_regA, 
                        pending_mem_address - DATA_SPACE);
                
                // Operation complete, reset state
                memory_busy = false;
                memory_delay = 0;
                memory_operation_in_progress = false; // Release pipeline freeze
            }
            else if (pending_opcode == 0b1010) { // SW - Store Word completed
                // Complete the store
                if (memory_target_delay == 1) {
                    // Cache hit
                    printf("[MEM_CACHE_HIT] Writing to cache address %u\n", pending_mem_address);
                    write_through(cache, &dram, pending_mem_address, pending_value);
                } else {
                    // DRAM access
                    printf("[MEM_DRAM_ACCESS] Writing to DRAM address %u\n", pending_mem_address);
                    writeToMemory(&dram, pending_mem_address, pending_value);
                }
                
                printf("[MEM_STORE_COMPLETE] Write to address %u, value %u complete\n", 
                       pending_mem_address, pending_value);
                       
                // Report memory update to UI
                printf("[MEM]%d:%d\n", pending_mem_address, pending_value);
                sprintf(instruction_text, "SW [R%d + %d], R%d", pending_regA, 
                        pending_mem_address - DATA_SPACE, pending_regD);
                
                // Operation complete, reset state
                memory_busy = false;
                memory_delay = 0;
                memory_operation_in_progress = false; // Release pipeline freeze
            }
        } else {
            // Still waiting for memory operation to complete
            if (pending_opcode == 0b1001) {
                sprintf(instruction_text, "LW R%d, [R%d + %d] (wait %d/%d)", 
                        pending_regD, pending_regA, pending_mem_address - DATA_SPACE, 
                        memory_delay, memory_target_delay);
            } else {
                sprintf(instruction_text, "SW [R%d + %d], R%d (wait %d/%d)", 
                        pending_regA, pending_mem_address - DATA_SPACE, pending_regD, 
                        memory_delay, memory_target_delay);
            }
            
            // Insert bubble into pipeline since we're waiting
            pipeline->MEM_WB_next.valid = false;
        }
    }
    // No ongoing memory operation, check if we need to start one
    else if (opcode == 5 || opcode == 9) { // LW - Load Word (both old and new opcodes)
        // Calculate effective address
        uint16_t mem_address = address;
        if (address < DATA_SPACE) {
            mem_address += DATA_SPACE;
        }
        
        // Start memory access with appropriate delay
        memory_busy = true;
        memory_delay = 0;
        memory_operation_in_progress = true; // Set global flag to freeze pipeline
        
        // Check if data is in cache
        bool cache_hit = false;
        if (cache != NULL) {
            // Check if the data is in cache
            uint16_t index = (mem_address / BLOCK_SIZE) % cache->num_sets;
            uint16_t tag = mem_address / (BLOCK_SIZE * cache->num_sets);
            uint16_t offset = mem_address & (BLOCK_SIZE - 1);
            
            Set *set = &cache->sets[index];
            for (int i = 0; i < cache->mode; i++) {
                if (set->lines[i].valid && set->lines[i].tag == tag) {
                    cache_hit = true;
                    break;
                }
            }
        }
        
        // Set delay based on cache hit or miss
        if (cache_hit) {
            memory_target_delay = 1; // 1 cycle for cache hit
            printf("[MEM_START] Cache hit for address %u, delay = %u cycles\n", 
                   mem_address, memory_target_delay);
        } else {
            memory_target_delay = 4; // 4 cycles for DRAM access
            printf("[MEM_START] Cache miss for address %u, delay = %u cycles\n", 
                   mem_address, memory_target_delay);
        }
        
        // Store pending operation details
        pending_mem_address = mem_address;
        pending_opcode = opcode;
        pending_regD = regD;
        pending_regA = regA;
        
        // Report memory operation started
        sprintf(instruction_text, "LW R%d, [R%d + %d] (wait 0/%d)", 
                regD, regA, address, memory_target_delay);
                
        // Insert bubble into pipeline since we're waiting
        pipeline->MEM_WB_next.valid = false;
    }
    else if (opcode == 4 || opcode == 10) { // SW - Store Word (both old and new opcodes)
        // Calculate effective address
        uint16_t mem_address = address;
        if (address < DATA_SPACE) {
            mem_address += DATA_SPACE;
        }
        
        // Get the value to store from regD
        uint16_t value_to_store = registers->R[regD];
        
        // Start memory access with appropriate delay
        memory_busy = true;
        memory_delay = 0;
        memory_operation_in_progress = true; // Set global flag to freeze pipeline
        
        // Check if data is in cache
        bool cache_hit = false;
        if (cache != NULL) {
            // Check if the location is in cache
            uint16_t index = (mem_address / BLOCK_SIZE) % cache->num_sets;
            uint16_t tag = mem_address / (BLOCK_SIZE * cache->num_sets);
            
            Set *set = &cache->sets[index];
            for (int i = 0; i < cache->mode; i++) {
                if (set->lines[i].valid && set->lines[i].tag == tag) {
                    cache_hit = true;
                    break;
                }
            }
        }
        
        // Set delay based on cache hit or miss
        if (cache_hit) {
            memory_target_delay = 1; // 1 cycle for cache hit
            printf("[MEM_START] Cache hit for address %u, delay = %u cycles\n", 
                   mem_address, memory_target_delay);
        } else {
            memory_target_delay = 4; // 4 cycles for DRAM access
            printf("[MEM_START] Cache miss for address %u, delay = %u cycles\n", 
                   mem_address, memory_target_delay);
        }
        
        // Store pending operation details
        pending_mem_address = mem_address;
        pending_opcode = opcode;
        pending_regD = regD;
        pending_regA = regA;
        pending_value = value_to_store;
        
        // Report memory operation started
        sprintf(instruction_text, "SW [R%d + %d], R%d (wait 0/%d)", 
                regA, address, regD, memory_target_delay);
                
        // Insert bubble into pipeline since we're waiting
        pipeline->MEM_WB_next.valid = false;
    }
    else {
        // For non-memory operations, ensure pipeline is not frozen
        memory_operation_in_progress = false;
        
        // Generate text for ALU operations
        switch(opcode) {
            case 0: sprintf(instruction_text, "ADD R%d, R%d, R%d", regD, regA, regB); break;
            case 1: sprintf(instruction_text, "SUB R%d, R%d, R%d", regD, regA, regB); break;
            case 2: sprintf(instruction_text, "NAND R%d, R%d, R%d", regD, regA, regB); break;
            case 3: sprintf(instruction_text, "LUI R%d, %d", regD, regB); break;
            case 6: sprintf(instruction_text, "BEQ R%d, R%d, %d", regD, regA, pipeline->MEM_WB.imm); break;
            case 7: sprintf(instruction_text, "JALR R%d, R%d", regD, regA); break;
            default: sprintf(instruction_text, "UNKNOWN opcode=%d", opcode);
        }
    }
    
    // Report pipeline state for UI
    if (!pipeline->EX_MEM.valid || pipeline->EX_MEM.opcode == 0 || pipeline->MEM_WB_next.valid == false) {
        printf("[PIPELINE]MEMORY:Bubble:%d\n", pipeline->EX_MEM.pc);
    } else {
        printf("[PIPELINE]MEMORY:%s:%d\n", instruction_text, pipeline->EX_MEM.pc);
    }
    fflush(stdout);
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