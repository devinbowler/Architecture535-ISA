# ARCH-16 CPU Simulator

## General Purpose and Features  
Our architecture is a **general-purpose** RISC-based design. Its distinguishing features include a **16-bit word size**, and a **unified memory model**. The instruction set is designed for simplicity and efficiency, incorporating basic **arithmetic, logical operations, memory access, and control flow instructions**.  

Our simulator will provide **step-by-step execution visualization** via a **graphical user interface (GUI)**. The GUI will support **register viewing, memory monitoring, cycle counting, and execution control**.  

---

## Word Size and Data Types  
- **Word Size**: **16-bit**
- **Data Types Supported**: 16-bit signed integers  
- **Addressing**: Word-addressable memory  

---

## Registers  
Our architecture includes the following registers:  

| Register | Name  | Purpose  |
|----------|------|----------|
| R0       | Zero | Always 0 (hardwired) |
| R1       | g0   | General register |
| R2       | g0   | General register |
| R3       | g0   | General register |
| R4       | g0   | General register |
| R5       | g1   | General register |
| R6       | g2   | General register |
| R7       | g3   | General register |
| R8       | g4   | General register |
| R9       | g5   | General register |
| R10      | g6   | General register |
| R11      | g7   | General register |
| R12      | g8   | General register |
| R13      | g9   | General register |
| R14      | sp   | Stack pointer (grows downward) |
| PC       | pc   | Program Counter |

---

## Execution Model  
- **Instruction Fetch Model**: One instruction per word  
- **Memory Organization**: **Unified memory model** (Princeton Architecture)  
- **Memory Addressing**: Direct memory addressing using **16-bit addresses**  
- **Addressable Memory Space**: **64 KiB (0x0000 to 0xFFFF)**  

---

## Instruction Set  

### Integer ALU Instructions  
| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0000    | ADD Rd, Ra, Rb | Rd = Ra + Rb |
| 0001    | SUB Rd, Ra, Rb | Rd = Ra - Rb |
| 0010    | AND Rd, Ra, Rb | Rd = Ra & Rb |
| 0011    | OR Rd, Ra, Rb  | Rd = Ra \| Rb |
| 0100    | XOR Rd, Ra, Rb  | Rd = Ra ^ Rb |

### Register-Type-Register-Immediate Instructions  
| Opcode | Type | Source Register | Immediate | Mnemonic | Description |
|--------|------|----------------|-----------|----------|-------------|
| 0101   | 00   | Ra             | imm[5:0]  | LSL Ra, imm | Logical Shift Left |
| 0101   | 01   | Ra             | imm[5:0]  | LSR Ra, imm | Logical Shift Right |
| 0101   | 10   | Ra             | imm[5:0]  | ROL Ra, imm | Rotate Left |
| 0101   | 11   | Ra             | imm[5:0]  | ROR Ra, imm | Rotate Right |

### Load/Store Instructions  
| Opcode | Register | Immediate | Mnemonic | Description |
|--------|----------|-----------|----------|-------------|
| 0110   | Rd      | imm[7:0]   | LW Rd, imm  | Load word from memory into register |
| 0111   | Rd      | imm[7:0]   | SW Rd, imm  | Store word from register into memory |

### Immediate Control Flow Instructions  
| Opcode | Immediate | Mnemonic | Description |
|--------|-----------|----------|-------------|
| 1000   | Addr[11:0]  | JMP Addr  | Unconditional jump |
| 1001   | Addr[11:0]  | CALL Addr | Function call |

### Special Instructions  
| Opcode | Immediate | Mnemonic | Description |
|--------|-----------|----------|-------------|
| 1010   | -         | RET  | Return from function |
| 1011   | -         | HALT | Stop execution |


---

## Memory Subsystem  
- **Memory Size**: 64 KiB  
- **Stack Memory**: Grows downward from `0x9000`  
- **Heap Memory**: Grows upward from `0x4000`  
- **Cache**: 4 KB **direct-mapped** cache  
  - **Write-through policy**, no-allocate on writes  
  - **Line size**: 4 words (8 bytes)  
  - **Latency**: **100 cycles (no cache)**, **1-2 cycles (cache hit)**  

---

## Pipeline Design  
The CPU follows a **5-stage pipeline**:  
1. **Instruction Fetch (IF)** - Retrieve instruction from memory  
2. **Instruction Decode (ID)** - Determine operation and fetch registers  
3. **Execute (EX)** - Perform ALU operation or branch decision  
4. **Memory (MEM)** - Load/store memory if applicable  
5. **Writeback (WB)** - Store results in registers  

---

## Simulator Features  
The simulator will provide:  
- **Graphical User Interface (GUI)** for easy execution tracking  
- **Register and Memory Display** (view registers in hexadecimal)  
- **Execution Control**:  
  - Single-step execution  
  - Run to completion  
- **Pipeline Execution Mode**:  
  - No cache, no pipeline  
  - Cache only  
  - Pipeline only  
  - Full cache and pipeline  

---

## Benchmarks  
The simulator will be tested with:  
1. **Exchange Sort** (to evaluate memory and ALU performance)  
2. **Matrix Multiplication** (to stress cache and pipeline performance)  

---

## Project Management Plan  
### Development Tools  
- **Programming Language**: C
- **GUI Framework**: Qt (Maybe nuetron)  
- **Version Control**: GitHub  
- **Collaboration Tools**: GitHub, Email 

### Development Timeline  
#### **Phase 1: Define ISA and Write Specification**  
- Finalize instruction set, memory layout, and execution model  
- Write documentation  

#### **Phase 2: Implement Assembler**  
- Convert assembly code to binary format  

#### **Phase 3: Implement CPU Core**  
- Implement register file, memory, and execution loop  

#### **Phase 4: Implement Pipelining and Cache**  
- Implement pipeline and cache logic  

#### **Phase 5: Implement GUI**  
- Display CPU state (registers, memory, pipeline visualization)  

#### **Phase 6: Run Benchmarks and Debug**  
- Execute sorting and matrix multiplication programs  

---

## Responsibilities  
| Team Member | Responsibilities |
|-------------|----------------|
| [Team Member 1] | TBD |
| [Team Member 2] | TBD |
