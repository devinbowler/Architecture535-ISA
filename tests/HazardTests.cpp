// #include <gtest/gtest.h>
// #include <stdint.h>
// #include <stdbool.h>
// extern "C" {
// #include "../simulator/pipeline.h"
// #include "../simulator/memory.h"
// #include "../simulator/hazards.h"
// #include "../simulator/pipeline/fetch.h"
// #include "../simulator/pipeline/decode.h"
// #include "../simulator/pipeline/execute.h"
// #include "../simulator/pipeline/memory_access.h"
// #include "../simulator/pipeline/write_back.h"
// }

// bool a_branch_taken;
// bool data_stall;
// uint16_t a_stall_cycles_remaining;
// DRAM dram;
// REGISTERS registers_instance;
// REGISTERS *registers = &registers_instance;
// Cache *cache = NULL;

// class PipelineHazardTest : public ::testing::Test {
// protected:
//     PipelineState pipeline;
//     uint16_t value; // Added value parameter for pipeline_step
    
//     void SetUp() override {
//         // Initialize memory and registers
//         registers = init_registers();
        
//         // Initialize DRAM
//         memset(&dram, 0, sizeof(DRAM));
//         dram.state = DRAM_IDLE;
//         dram.delayCounter = 0;
//         clearMemory(&dram);
        
//         // Initialize pipeline state
//         memset(&pipeline, 0, sizeof(PipelineState));
        
//         // Reset branch and stall flags
//         a_branch_taken = false;
//         data_stall = false;
//         a_stall_cycles_remaining = 0;
        
//         // Set up PC to 0
//         registers->R[15] = 0;
        
//         // Initialize value parameter
//         value = 0;
//     }
    
//     void TearDown() override {
//         free(registers);
//     }
    
//     // Helper to load test program into memory
//     void LoadProgram(uint16_t* instructions, int count) {
//         for (int i = 0; i < count; i++) {
//             writeToMemory(&dram, i, instructions[i]);
//         }
//     }
    
//     // Helper to run pipeline for n cycles
//     void RunPipelineCycles(int cycles) {
//         for (int i = 0; i < cycles; i++) {
//             pipeline_step(&pipeline, &value);
//         }
//     }
    
//     // Helper to examine hazards directly
//     HazardInfo CheckForHazards() {
//         return detect_hazards(&pipeline);
//     }
// };

// // Test RAW hazard with forwarding from EX/MEM to ID/EX (for regA)
// TEST_F(PipelineHazardTest, ForwardingFromEXMEMtoRegA) {
//     // Test program:
//     // ADD R1, R2, R3     ; R1 = R2 + R3
//     // ADD R4, R1, R5     ; R4 = R1 + R5 (RAW hazard on R1)
    
//     // Initialize registers with test values
//     registers->R[2] = 10;
//     registers->R[3] = 5;
//     registers->R[5] = 3;
    
//     // Create program instructions
//     uint16_t instructions[] = {
//         0x0123, // ADD R1, R2, R3
//         0x0415  // ADD R4, R1, R5
//     };
    
//     LoadProgram(instructions, 2);
    
//     // Execute fetch and decode for first instruction
//     pipeline_step(&pipeline, &value);
    
//     // Execute fetch for second instruction and decode+execute for first
//     pipeline_step(&pipeline, &value);
    
//     // At this point, first instruction is in EX/MEM and second is in ID/EX
//     // Check for hazards
//     HazardInfo hazard = CheckForHazards();
//     EXPECT_TRUE(hazard.detected);
//     EXPECT_FALSE(hazard.requires_stall);
//     EXPECT_EQ(hazard.source_stage, 1); // EX/MEM stage
    
//     // Continue pipeline execution
//     pipeline_step(&pipeline, &value);
//     pipeline_step(&pipeline, &value);
    
//     // Verify the result - R1 should be 15 and R4 should be 18 if forwarding worked
//     EXPECT_EQ(registers->R[1], 15); // R1 = 10 + 5
//     EXPECT_EQ(registers->R[4], 18); // R4 = 15 + 3
// }

// // Test RAW hazard with forwarding from EX/MEM to ID/EX (for regB)
// TEST_F(PipelineHazardTest, ForwardingFromEXMEMtoRegB) {
//     // Test program:
//     // ADD R1, R2, R3     ; R1 = R2 + R3
//     // ADD R4, R5, R1     ; R4 = R5 + R1 (RAW hazard on R1)
    
//     // Initialize registers with test values
//     registers->R[2] = 10;
//     registers->R[3] = 5;
//     registers->R[5] = 3;
    
//     // Create program instructions
//     uint16_t instructions[] = {
//         0x0123, // ADD R1, R2, R3
//         0x0451  // ADD R4, R5, R1
//     };
    
//     LoadProgram(instructions, 2);
    
//     // Run pipeline for 2 cycles to get the first instruction to EX/MEM
//     // and the second to ID/EX
//     pipeline_step(&pipeline, &value);
//     pipeline_step(&pipeline, &value);
    
//     // Check for hazards
//     HazardInfo hazard = CheckForHazards();
//     EXPECT_TRUE(hazard.detected);
//     EXPECT_FALSE(hazard.requires_stall);
//     EXPECT_EQ(hazard.source_stage, 1); // EX/MEM stage
    
//     // Continue pipeline execution
//     pipeline_step(&pipeline, &value);
//     pipeline_step(&pipeline, &value);
    
//     // Verify the result - R4 should be 18 (3+15) if forwarding worked
//     EXPECT_EQ(registers->R[1], 15); // R1 = 10 + 5
//     EXPECT_EQ(registers->R[4], 18); // R4 = 3 + 15
// }

// // Test RAW hazard with forwarding from MEM/WB to ID/EX
// TEST_F(PipelineHazardTest, ForwardingFromMEMWBtoIDEX) {
//     // Test program:
//     // ADD R1, R2, R3     ; R1 = R2 + R3
//     // ADD R6, R7, R8     ; Unrelated instruction
//     // ADD R4, R1, R5     ; R4 = R1 + R5 (RAW hazard on R1 from two instructions ago)
    
//     // Initialize registers with test values
//     registers->R[2] = 10;
//     registers->R[3] = 5;
//     registers->R[5] = 3;
//     registers->R[7] = 1;
//     registers->R[8] = 2;
    
//     // Create program instructions
//     uint16_t instructions[] = {
//         0x0123, // ADD R1, R2, R3
//         0x0678, // ADD R6, R7, R8
//         0x0415  // ADD R4, R1, R5
//     };
    
//     LoadProgram(instructions, 3);
    
//     // Run pipeline for 3 cycles to get first instruction to MEM/WB,
//     // second to EX/MEM, and third to ID/EX
//     pipeline_step(&pipeline, &value);
//     pipeline_step(&pipeline, &value);
//     pipeline_step(&pipeline, &value);
    
//     // Check for hazards
//     HazardInfo hazard = CheckForHazards();
//     EXPECT_TRUE(hazard.detected);
//     EXPECT_FALSE(hazard.requires_stall);
//     EXPECT_EQ(hazard.source_stage, 2); // MEM/WB stage
    
//     // Continue pipeline execution
//     pipeline_step(&pipeline, &value);
//     pipeline_step(&pipeline, &value);
    
//     // Verify the results
//     EXPECT_EQ(registers->R[1], 15); // R1 = 10 + 5
//     EXPECT_EQ(registers->R[6], 3);  // R6 = 1 + 2
//     EXPECT_EQ(registers->R[4], 18); // R4 = 15 + 3
// }

// // Test Load-Use Hazard requiring stall
// TEST_F(PipelineHazardTest, LoadUseHazardStall) {
//     // Test program:
//     // LW R1, [R2+1]      ; Load a value into R1
//     // ADD R4, R1, R5     ; Use R1 immediately (requires stall)
    
//     // Initialize registers and memory
//     registers->R[2] = DATA_SPACE;  // Base address for memory access
//     registers->R[5] = 3;
    
//     // Put a value in memory to be loaded
//     writeToMemory(&dram, DATA_SPACE+1, 15);
    
//     // Create program instructions
//     uint16_t instructions[] = {
//         0x9121, // LW R1, [R2+1]
//         0x0415  // ADD R4, R1, R5
//     };
    
//     LoadProgram(instructions, 2);
    
//     // Run pipeline for 2 cycles to get load instruction to EX/MEM
//     // and ADD instruction to ID/EX
//     pipeline_step(&pipeline, &value);
//     pipeline_step(&pipeline, &value);
    
//     // Check for hazards
//     HazardInfo hazard = CheckForHazards();
//     EXPECT_TRUE(hazard.detected);
//     EXPECT_TRUE(hazard.requires_stall);
//     EXPECT_EQ(hazard.stall_cycles, 1);
    
//     // Continue pipeline execution to resolve stall
//     pipeline_step(&pipeline, &value);
//     pipeline_step(&pipeline, &value);
//     pipeline_step(&pipeline, &value);
    
//     // Verify the results - R4 should be 18 (15+3) after stall is resolved
//     EXPECT_EQ(registers->R[1], 15); // R1 loaded from memory
//     EXPECT_EQ(registers->R[4], 18); // R4 = 15 + 3
    
//     // Verify stall was triggered and completed
//     EXPECT_FALSE(data_stall); // Stall should be finished by now
// }

// // Test multiple hazards in sequence
// TEST_F(PipelineHazardTest, MultipleHazardsSequence) {
//     // Test program:
//     // ADD R1, R2, R3     ; R1 = R2 + R3
//     // ADD R4, R1, R5     ; R4 = R1 + R5 (RAW hazard on R1)
//     // ADD R6, R4, R7     ; R6 = R4 + R7 (RAW hazard on R4)
    
//     // Initialize registers with test values
//     registers->R[2] = 10;
//     registers->R[3] = 5;
//     registers->R[5] = 3;
//     registers->R[7] = 2;
    
//     // Create program instructions
//     uint16_t instructions[] = {
//         0x0123, // ADD R1, R2, R3
//         0x0415, // ADD R4, R1, R5
//         0x0647  // ADD R6, R4, R7
//     };
    
//     LoadProgram(instructions, 3);
    
//     // Run pipeline for enough cycles to execute all instructions
//     RunPipelineCycles(6);
    
//     // Verify the results
//     EXPECT_EQ(registers->R[1], 15); // R1 = 10 + 5
//     EXPECT_EQ(registers->R[4], 18); // R4 = 15 + 3
//     EXPECT_EQ(registers->R[6], 20); // R6 = 18 + 2
// }

// // Test branch with hazards
// TEST_F(PipelineHazardTest, BranchWithHazard) {
//     // Test program:
//     // ADD R1, R2, R3     ; R1 = R2 + R3
//     // BEQ R1, R3, 2      ; Branch if R1 == R3 (RAW hazard on R1)
//     // ADD R4, R5, R6     ; Instruction might be skipped
//     // ADD R7, R8, R9     ; Instruction might be skipped
//     // ADD R10, R11, R12  ; Target of branch if taken
    
//     // Initialize registers with test values - branch not taken
//     registers->R[2] = 0;
//     registers->R[3] = 5;  // R1 will be 5 != R3 (5), so branch not taken
//     registers->R[5] = 10;
//     registers->R[6] = 20;
//     registers->R[8] = 30;
//     registers->R[9] = 40;
//     registers->R[11] = 50;
//     registers->R[12] = 60;
    
//     // Create program instructions
//     uint16_t instructions[] = {
//         0x0123, // ADD R1, R2, R3
//         0xB132, // BEQ R1, R3, 2 
//         0x0456, // ADD R4, R5, R6
//         0x0789, // ADD R7, R8, R9
//         0x0ABC  // ADD R10, R11, R12
//     };
    
//     LoadProgram(instructions, 5);
    
//     // Run pipeline to execute all instructions
//     RunPipelineCycles(8);
    
//     // Verify the results for branch not taken path
//     EXPECT_EQ(registers->R[1], 5);    // R1 = 0 + 5
//     EXPECT_EQ(registers->R[4], 30);   // R4 = 10 + 20 (executed)
//     EXPECT_EQ(registers->R[7], 70);   // R7 = 30 + 40 (executed)
//     EXPECT_EQ(registers->R[10], 110); // R10 = 50 + 60 (executed)
    
//     // Reset and test with branch taken path
//     SetUp(); // Reset pipeline and registers
    
//     // Set values for branch to be taken
//     registers->R[2] = 5;  // Will make R1 = 10
//     registers->R[3] = 10; // R1 == R3, so branch should be taken
//     registers->R[5] = 10;
//     registers->R[6] = 20;
//     registers->R[8] = 30;
//     registers->R[9] = 40;
//     registers->R[11] = 50;
//     registers->R[12] = 60;
    
//     // Load the same program
//     LoadProgram(instructions, 5);
    
//     // Run pipeline to execute all instructions
//     RunPipelineCycles(8);
    
//     // Verify branch was taken
//     EXPECT_EQ(registers->R[1], 15);   // R1 = 5 + 10
//     // R4 and R7 might be modified due to pipeline behavior with branches
//     // The exact behavior depends on your branch prediction/handling
//     EXPECT_EQ(registers->R[10], 110); // R10 = 50 + 60 (target of branch)
// }

// // Test combined memory and hazard stalls
// TEST_F(PipelineHazardTest, MemoryAndHazardStallInteraction) {
//     // Create a cache for testing memory delays
//     cache = init_cache(1); // Direct-mapped mode
    
//     // Test program:
//     // LW R1, [R2+1]      ; Load a value into R1
//     // ADD R4, R1, R5     ; Use R1 immediately (requires stall)
    
//     // Initialize registers and memory
//     registers->R[2] = DATA_SPACE;  // Base address for memory access
//     registers->R[5] = 3;
    
//     // Put a value in memory to be loaded
//     writeToMemory(&dram, DATA_SPACE+1, 15);
    
//     // Create program instructions
//     uint16_t instructions[] = {
//         0x9121, // LW R1, [R2+1]
//         0x0415  // ADD R4, R1, R5
//     };
    
//     LoadProgram(instructions, 2);
    
//     // Run pipeline for enough cycles to handle both memory and hazard stalls
//     RunPipelineCycles(10);
    
//     // Verify the results
//     EXPECT_EQ(registers->R[1], 15); // R1 loaded from memory
//     EXPECT_EQ(registers->R[4], 18); // R4 = 15 + 3
    
//     // Clean up cache
//     destroy_cache(cache);
//     cache = NULL;
// }

// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }