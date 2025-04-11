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
Cache *cache;

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
        
        // Initialize memory
        // Add any memory setup here
        
        // Reset pipeline stages
        pipeline.IF_ID.valid = false;
        pipeline.ID_EX.valid = false;
        pipeline.EX_MEM.valid = false;
        pipeline.MEM_WB.valid = false;
        
        pipeline.IF_ID_next.valid = false;
        pipeline.ID_EX_next.valid = false;
        pipeline.EX_MEM_next.valid = false;
        pipeline.MEM_WB_next.valid = false;
    }
};

// Test the fetch stage
TEST_F(PipelineTest, FetchStageTest) {
    // Setup
    uint16_t instruction = 0x1234; // Example instruction
    pipeline.IF_ID.valid = true;   // We're ready to fetch
    registers->R[15] = 0x100;      // PC value
    
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
    
    // Set register values
    registers->R[2] = 42;  // R2 value
    registers->R[3] = 24;  // R3 value
    
    // Execute decode stage
    decode_stage(&pipeline);
    
    // Verify
    EXPECT_TRUE(pipeline.ID_EX_next.valid);
    EXPECT_EQ(pipeline.ID_EX_next.opcode, 0);     // ADD
    EXPECT_EQ(pipeline.ID_EX_next.regD, 1);       // R1
    EXPECT_EQ(pipeline.ID_EX_next.regA, 42);      // Value from R2
    EXPECT_EQ(pipeline.ID_EX_next.regB, 24);      // Value from R3
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
    
    pipeline.EX_MEM.valid = true;  // Ready for execution
    
    // Execute
    execute(&pipeline);
    
    // Verify
    EXPECT_TRUE(pipeline.EX_MEM_next.valid);
    EXPECT_EQ(pipeline.EX_MEM_next.res, 66);  // 42 + 24 = 66
    EXPECT_EQ(pipeline.EX_MEM_next.regD, 0);  // regD value passed through
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
    
    pipeline.EX_MEM.valid = true;  // Ready for execution
    
    // Execute
    execute(&pipeline);
    
    // Verify
    EXPECT_TRUE(pipeline.EX_MEM_next.valid);
    EXPECT_EQ(pipeline.EX_MEM_next.res, 30);  // 50 - 20 = 30
}

// Test memory access stage with LW instruction
TEST_F(PipelineTest, MemoryAccessLoadTest) {
    // Setup - LW operation
    pipeline.EX_MEM.valid = true;
    pipeline.EX_MEM.opcode = 9;    // LW (0b1001)
    pipeline.EX_MEM.regD = 5;      // Target register R5
    pipeline.EX_MEM.res = 0x200;   // Memory address
    
    // Mock memory read
    // Need to set up a way to return a value from readFromMemory
    // This might require modifying the code or creating a test double
    
    // Execute
    memory_access(&pipeline);
    
    // Verify
    EXPECT_TRUE(pipeline.MEM_WB_next.valid);
    EXPECT_EQ(pipeline.MEM_WB_next.regD, 5);  // Register passed through
    // Would verify the result if we could mock readFromMemory
}

// Test memory access stage with SW instruction
TEST_F(PipelineTest, MemoryAccessStoreTest) {
    // Setup - SW operation
    pipeline.EX_MEM.valid = true;
    pipeline.EX_MEM.opcode = 10;   // SW (0b1010)
    pipeline.EX_MEM.regD = 5;      // Source register R5
    pipeline.EX_MEM.regB = 42;     // Value to store
    pipeline.EX_MEM.res = 0x200;   // Memory address
    
    // Execute
    memory_access(&pipeline);
    
    // Verify
    EXPECT_TRUE(pipeline.MEM_WB_next.valid);
    // Would verify the memory write if we could check DRAM
}

// Test write back stage with ADD result
TEST_F(PipelineTest, WriteBackAddTest) {
    // Setup - Result of ADD
    pipeline.MEM_WB.valid = true;
    pipeline.MEM_WB.opcode = 0;    // ADD
    pipeline.MEM_WB.regD = 4;      // Target register R4
    pipeline.MEM_WB.res = 66;      // Result value
    
    registers->R[4] = 0;           // Initial register value
    
    // Execute
    write_back(&pipeline);
    
    // Verify
    EXPECT_EQ(registers->R[4], 66);  // Register updated with result
}

// Test write back stage with DIVMOD result
TEST_F(PipelineTest, WriteBackDivModTest) {
    // Setup - Result of DIVMOD
    pipeline.MEM_WB.valid = true;
    pipeline.MEM_WB.opcode = 5;    // DIVMOD
    pipeline.MEM_WB.regD = 4;      // Quotient register R4
    pipeline.MEM_WB.regB = 5;      // Remainder register R5
    pipeline.MEM_WB.res = 3;       // Quotient
    pipeline.MEM_WB.resMod = 1;    // Remainder
    
    registers->R[4] = 0;           // Initial quotient register
    registers->R[5] = 0;           // Initial remainder register
    
    // Execute
    write_back(&pipeline);
    
    // Verify
    EXPECT_EQ(registers->R[4], 3);  // Quotient register updated
    EXPECT_EQ(registers->R[5], 1);  // Remainder register updated
}

// Test full pipeline with ADD instruction
TEST_F(PipelineTest, FullPipelineAddTest) {
    // Setup initial state
    uint16_t instruction = 0x0123; // ADD R1, R2, R3
    registers->R[2] = 42;          // Value in R2
    registers->R[3] = 24;          // Value in R3
    registers->R[15] = 0x100;      // PC
    
    // Pipeline stages
    pipeline.IF_ID.valid = true;
    
    // Step 1: Fetch
    fetch_stage(&pipeline, &instruction);
    // Update pipeline state
    pipeline.IF_ID = pipeline.IF_ID_next;
    
    // Step 2: Decode
    decode_stage(&pipeline);
    // Update pipeline state
    pipeline.ID_EX = pipeline.ID_EX_next;
    
    // Step 3: Execute
    pipeline.EX_MEM.valid = true;  // Ready for execution
    execute(&pipeline);
    // Update pipeline state
    pipeline.EX_MEM = pipeline.EX_MEM_next;
    
    // Step 4: Memory Access
    memory_access(&pipeline);
    // Update pipeline state
    pipeline.MEM_WB = pipeline.MEM_WB_next;
    
    // Step 5: Write Back
    write_back(&pipeline);
    
    // Verify final result
    EXPECT_EQ(registers->R[1], 66);  // 42 + 24 = 66
}

// Test BEQ instruction (branch if equal)
TEST_F(PipelineTest, BranchIfEqualTest) {
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
    
    pipeline.EX_MEM.valid = true;  // Ready for execution
    
    // Execute
    execute(&pipeline);
    
    // Verify PC is updated to take the branch
    EXPECT_EQ(registers->R[15], 0x105);  // PC + offset
    
    // Now test when condition is false
    registers->R[1] = 42;
    registers->R[2] = 24;          // Different value
    registers->R[15] = 0x100;      // Reset PC
    
    // Execute
    execute(&pipeline);
    
    // Verify PC is incremented normally
    EXPECT_EQ(registers->R[15], 0x101);  // PC + 1
}

// Test full pipeline integration
TEST_F(PipelineTest, PipelineIntegrationTest) {
    // This test would simulate multiple clock cycles of the pipeline
    // and verify that instructions flow through all stages correctly
    
    // Add test implementation here
    // This would be more complex and might require additional setup
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
