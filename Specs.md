==============================
#         ARCH-8 SPEC       
==============================

## OVERVIEW
* 8-bit Reduced Instruction Set Computing (RISC) architecture
* 16-bit memory address space (64 KiB accessible memory)
* Unified memory model (instructions and data share the same memory space)
* Supports a 5-stage pipeline: Fetch, Decode, Execute, Memory, Writeback

## INSTRUCTIONS
0: ADD  Rd, Rs       -> Rd = Rd + Rs  
1: SUB  Rd, Rs       -> Rd = Rd - Rs  
2: AND  Rd, Rs       -> Rd = Rd & Rs  
3: OR   Rd, Rs       -> Rd = Rd | Rs  
4: LW   Rd, [Addr]   -> Rd = Memory[Addr]  
5: SW   [Addr], Rs   -> Memory[Addr] = Rs  
6: BEQ  Rs, Rt, imm  -> If Rs == Rt, PC += imm  
7: BNE  Rs, Rt, imm  -> If Rs != Rt, PC += imm  
8: J    Addr         -> PC = Addr  
9: HALT              -> Stop execution  

* `Addr` is a 16-bit memory address.  
* `imm` is an 8-bit immediate value for branching.  

## REGISTERS
R0  (zero): Always 0  
R1  (v0): Function return value  
R2  (a0): Function argument  
R3  (a1): Function argument  
R4  (t0): Temporary register  
R5  (t1): Temporary register  
R6  (sp): Stack pointer  
R7  (sr): Status register (Zero, Carry, Overflow)  
PC  (pc): Program Counter  

## INSTRUCTION LAYOUT
Instructions are **16-bit fixed length**, using two formats:

1. **R-Type (Register-Register Instructions)**  
   - Format: `[Opcode(4)] [Rd(3)] [Rs(3)] [0000]`  
   - Example: `ADD R1, R2` -> `0000 001 010 0000`  

2. **I-Type (Immediate Instructions)**  
   - Format: `[Opcode(4)] [Rs(3)] [Rt(3)] [imm8(8)]`  
   - Example: `BEQ R1, R2, 10` -> `0110 001 010 00001010`  

## MEMORY LAYOUT
0x0000..0x3FFF: GENERAL PURPOSE ROM  
0x4000..0x7FFF: GENERAL PURPOSE RAM  
0x8000..0x8FFF: MEMORY-MAPPED I/O (MMIO)  
0x9000..0x9FFF: STACK MEMORY  

## MEMORY MANAGEMENT
- Unified memory space for instructions and data  
- Stack grows downward from `0x9000`  
- Heap grows upward from `0x4000`  

## PIPELINE DESIGN
The CPU follows a 5-stage pipeline:  
1. **Fetch (IF)**: Retrieve instruction from memory  
2. **Decode (ID)**: Determine operation and fetch registers  
3. **Execute (EX)**: Perform ALU operation or branch decision  
4. **Memory (MEM)**: Load/store to memory if applicable  
5. **Writeback (WB)**: Store results in registers  

## CACHE SYSTEM
- Unified L1 Cache (4 KB, direct-mapped, 16-byte blocks)  
- Write-through policy, no-allocate on write  
- Cache line size: 2 words (8 bytes)  
- Memory access latency: 100 cycles (without cache), 1-2 cycles (with cache hit)  

## SIMULATOR OVERVIEW
The ISA will be implemented in a software-based simulator that mimics CPU execution cycle by cycle.  
1. **Instruction Fetch**: Read from memory, increment PC  
2. **Decode & Execute**: Identify opcode and perform computation  
3. **Pipeline Execution**: Track instruction states through stages  
4. **Cache Simulation**: Track hit/miss rates  
5. **UI for Debugging**: Display registers, memory, pipeline states  

## BENCHMARK PROGRAMS
1. **Sorting Algorithm** (Exchange Sort) to test memory and ALU performance  
2. **Matrix Multiplication** to stress the cache and pipeline  

## EXTRA CREDIT CONSIDERATIONS
- Implement **branch prediction** to optimize jumps  
- Add **superscalar execution** (dual instruction issue)  
- Extend cache with **associative mapping**  

==============================
#       END OF ARCH-8 SPEC   
==============================
