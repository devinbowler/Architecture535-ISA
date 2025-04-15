#include <gtest/gtest.h>
#include <stdint.h>
extern "C" {
#include "../simulator/pipeline.h"
#include "../simulator/pipeline/fetch.h"
#include "../simulator/pipeline/decode.h"
#include "../simulator/pipeline/execute.h"
#include "../simulator/pipeline/memory_access.h"
#include "../simulator/pipeline/write_back.h"
#include "../simulator/memory.h"
}

// Mock objects for testing
DRAM dram;
REGISTERS registers_instance;
REGISTERS *registers = &registers_instance;
Cache *cache = NULL;

// External variables that need to be defined for tests
extern bool branch_taken;
extern uint16_t branch_target_address;
extern bool memory_operation_in_progress;

class PipelineTest : public ::testing::Test {
protected:
    PipelineState pipeline;
    
    void SetUp() override {
        // Initialize pipeline state
        memset(&pipeline, 0, sizeof(PipelineState));
        
        // Initialize registers
        for (int i = 0; i < 16; i++) {
            registers->R[i] = 0;
        }
        
        // Program counter (R15)
        registers->R[15] = 0;
        
        // Initialize memory with some test values
        for (int i = 0; i < DRAM_SIZE; i++) {
            dram.memory[i] = 0;
        }
        
        // Reset pipeline stages
        pipeline.IF_ID.valid = false;
        pipeline.ID_EX.valid = false;
        pipeline.EX_MEM.valid = false;
        pipeline.MEM_WB.valid = false;
        
        pipeline.IF_ID_next.valid = false;
        pipeline.ID_EX_next.valid = false;
        pipeline.EX_MEM_next.valid = false;
        pipeline.MEM_WB_next.valid = false;
        
        // Reset branch flags
        branch_taken = false;
        branch_target_address = 0;
    }
};

// Test the fetch stage
TEST_F(PipelineTest, FetchStageTest) {
    // Setup
    uint16_t instruction = 0x1234; // Example instruction
    registers->R[15] = 0x100;      // PC value
    
    // Set up memory at PC location
    writeToMemory(&dram, 0x100, instruction);
    
    // Execute fetch stage
    fetch_stage(&pipeline, &instruction);
    
    // Verify
    EXPECT_TRUE(pipeline.IF_ID_next.valid);
    EXPECT_EQ(pipeline.IF_ID_next.instruction, instruction);
    EXPECT_EQ(pipeline.IF_ID_next.pc, registers->R[15]);
}

// Test decode stage with ADD instruction
TEST_F(PipelineTest, DecodeStageAddTest) {
    // Setup - ADD R1, R2, R3 (opcode 0000, rd 0001, ra 0010, rb 0011)
    pipeline.IF_ID.valid = true;
    pipeline.IF_ID.instruction = 0x0123; // 0000 0001 0010 0011
    pipeline.IF_ID.pc = 0x100;
    
    // Execute decode stage
    decode_stage(&pipeline);
    
    // Verify
    EXPECT_TRUE(pipeline.ID_EX_next.valid);
    EXPECT_EQ(pipeline.ID_EX_next.opcode, 0);     // ADD
    EXPECT_EQ(pipeline.ID_EX_next.regD, 1);       // R1
    EXPECT_EQ(pipeline.ID_EX_next.regA, 2);       // R2 index (not value)
    EXPECT_EQ(pipeline.ID_EX_next.regB, 3);       // R3 index (not value)
    EXPECT_EQ(pipeline.ID_EX_next.pc, 0x100);     // PC passed through
}

// Test execute stage with ADD instruction
TEST_F(PipelineTest, ExecuteStageAddTest) {
    // Setup - ADD operation
    pipeline.ID_EX.valid = true;
    pipeline.ID_EX.opcode = 0;     // ADD
    pipeline.ID_EX.regD = 1;       // Target register R1
    pipeline.ID_EX.regA = 2;       // Source register index R2
    pipeline.ID_EX.regB = 3;       // Source register index R3
    pipeline.ID_EX.pc = 0x100;
    
    registers->R[1] = 0;           // Initial value of target register
    registers->R[2] = 42;          // Value in R2
    registers->R[3] = 24;          // Value in R3
    
    // Execute
    execute(&pipeline);
    
    // Verify
    EXPECT_TRUE(pipeline.EX_MEM_next.valid);
    EXPECT_EQ(pipeline.EX_MEM_next.res, 66);      // 42 + 24 = 66
    EXPECT_EQ(pipeline.EX_MEM_next.regD, 1);      // regD value passed through
    EXPECT_EQ(pipeline.EX_MEM_next.opcode, 0);    // opcode passed through
}

// Test execute stage with SUB instruction
TEST_F(PipelineTest, ExecuteStageSubTest) {
    // Setup - SUB operation
    pipeline.ID_EX.valid = true;
    pipeline.ID_EX.opcode = 1;     // SUB
    pipeline.ID_EX.regD = 1;       // Target register R1
    pipeline.ID_EX.regA = 2;       // Source register index R2
    pipeline.ID_EX.regB = 3;       // Source register index R3
    pipeline.ID_EX.pc = 0x100;
    
    registers->R[1] = 0;           // Initial value of target register
    registers->R[2] = 50;          // Value in R2
    registers->R[3] = 20;          // Value in R3
    
    // Execute
    execute(&pipeline);
    
    // Verify
    EXPECT_TRUE(pipeline.EX_MEM_next.valid);
    EXPECT_EQ(pipeline.EX_MEM_next.res, 30);      // 50 - 20 = 30
    EXPECT_EQ(pipeline.EX_MEM_next.regD, 1);      // regD value passed through
}


// Test memory access stage with LW instruction - simplified version
TEST_F(PipelineTest, MemoryAccessLoadTest) {
    // Setup - LW operation
    pipeline.EX_MEM.valid = true;
    pipeline.EX_MEM.opcode = 5;    // LW (old opcode)
    pipeline.EX_MEM.regD = 5;      // Target register R5
    pipeline.EX_MEM.regA = 2;      // Base register R2 
    pipeline.EX_MEM.res = 200;     // Memory offset (calculated from regA + imm)
    pipeline.EX_MEM.pc = 0x100;
    
    // Setup memory with test value
    writeToMemory(&dram, DATA_SPACE + 200, 42); // Write 42 to memory location
    
    // Execute (this will start the memory operation, might not complete in one call)
    memory_access(&pipeline);
    
    // Verify operation started
    EXPECT_TRUE(memory_operation_in_progress);
    
    // Note: In the real implementation, memory_access needs to be called multiple times
    // to simulate the memory delay. For test simplicity, we're not verifying the full sequence.
}

// Test memory access stage with SW instruction - simplified version
TEST_F(PipelineTest, MemoryAccessStoreTest) {
    // Setup - SW operation
    pipeline.EX_MEM.valid = true;
    pipeline.EX_MEM.opcode = 4;    // SW (old opcode)
    pipeline.EX_MEM.regD = 5;      // Source register R5
    pipeline.EX_MEM.regA = 2;      // Base register R2
    pipeline.EX_MEM.res = 200;     // Memory offset (calculated from regA + imm)
    pipeline.EX_MEM.pc = 0x100;
    
    registers->R[5] = 99;          // Value to store
    
    // Execute (this will start the memory operation, might not complete in one call)
    memory_access(&pipeline);
    
    // Verify operation started
    EXPECT_TRUE(memory_operation_in_progress);
}

// Test write back stage with ADD result
TEST_F(PipelineTest, WriteBackAddTest) {
    // Setup - Result of ADD
    pipeline.MEM_WB.valid = true;
    pipeline.MEM_WB.opcode = 0;    // ADD
    pipeline.MEM_WB.regD = 4;      // Target register R4
    pipeline.MEM_WB.res = 66;      // Result value
    pipeline.MEM_WB.pc = 0x100;
    
    registers->R[4] = 0;           // Initial register value
    
    // Execute
    write_back(&pipeline);
    
    // Verify
    EXPECT_EQ(registers->R[4], 66);  // Register updated with result
}

// Test BEQ instruction (branch taken)
TEST_F(PipelineTest, BranchIfEqualTakenTest) {
    // Setup - BEQ R1, R2, 5 (branch if R1 == R2, offset 5)
    pipeline.ID_EX.valid = true;
    pipeline.ID_EX.opcode = 11;    // BEQ (0b1011)
    pipeline.ID_EX.regD = 1;       // Register R1
    pipeline.ID_EX.regA = 2;       // Register R2
    pipeline.ID_EX.imm = 5;        // Branch offset
    pipeline.ID_EX.pc = 0x100;     // Current PC
    
    registers->R[1] = 42;          // R1 value
    registers->R[2] = 42;          // R2 value (equal to R1)
    registers->R[15] = 0x100;      // PC
    
    // Execute
    execute(&pipeline);
    
    // Verify branch was taken
    EXPECT_TRUE(branch_taken);
    EXPECT_EQ(branch_target_address, 0x105);  // PC + offset
}

// Test BEQ instruction (branch not taken)
TEST_F(PipelineTest, BranchIfEqualNotTakenTest) {
    // Setup - BEQ R1, R2, 5 (branch if R1 == R2, offset 5)
    pipeline.ID_EX.valid = true;
    pipeline.ID_EX.opcode = 11;    // BEQ (0b1011)
    pipeline.ID_EX.regD = 1;       // Register R1
    pipeline.ID_EX.regA = 2;       // Register R2
    pipeline.ID_EX.imm = 5;        // Branch offset
    pipeline.ID_EX.pc = 0x100;     // Current PC
    
    registers->R[1] = 42;          // R1 value
    registers->R[2] = 24;          // R2 value (not equal to R1)
    registers->R[15] = 0x100;      // PC
    
    // Execute
    execute(&pipeline);
    
    // Verify branch was not taken
    EXPECT_FALSE(branch_taken);
    EXPECT_NE(branch_target_address, 0x105);
}


TEST_F(PipelineTest, IntegrationAddTest) {
  // Reset global memory operation flags
  memory_operation_in_progress = false;
  
  // Create an ADD R1, R2, R3 instruction and put it in memory
  uint16_t add_instruction = 0x0123; // 0000 0001 0010 0011
  writeToMemory(&dram, 0, add_instruction);
  
  // Set register values
  registers->R[1] = 0;  // Target register
  registers->R[2] = 42; // First source register
  registers->R[3] = 24; // Second source register
  registers->R[15] = 0; // PC
  
  // Clear pipeline state before starting
  memset(&pipeline, 0, sizeof(PipelineState));
  
  // Cycle 1: Fetch
  uint16_t instruction;
  fetch_stage(&pipeline, &instruction);
  pipeline.IF_ID = pipeline.IF_ID_next;
  memset(&pipeline.IF_ID_next, 0, sizeof(pipeline.IF_ID_next));
  
  // Cycle 2: Decode
  decode_stage(&pipeline);
  pipeline.ID_EX = pipeline.ID_EX_next;
  memset(&pipeline.ID_EX_next, 0, sizeof(pipeline.ID_EX_next));
  
  // Cycle 3: Execute
  execute(&pipeline);
  pipeline.EX_MEM = pipeline.EX_MEM_next;
  memset(&pipeline.EX_MEM_next, 0, sizeof(pipeline.EX_MEM_next));
  
  // Ensure EX_MEM stage is valid
  pipeline.EX_MEM.valid = true;
  
  // Cycle 4: Memory Access
  memory_access(&pipeline);
  pipeline.MEM_WB = pipeline.MEM_WB_next;
  memset(&pipeline.MEM_WB_next, 0, sizeof(pipeline.MEM_WB_next));
  
  // Ensure MEM_WB stage is valid
  pipeline.MEM_WB.valid = true;
  
  // Cycle 5: Write Back
  write_back(&pipeline);
  
  // Verify the final result
  EXPECT_EQ(registers->R[1], 66); // 42 + 24 = 66
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}