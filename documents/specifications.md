# ARCH-16 Specification

## General Features

We decided to go for a general-purpose architecture. Its distinguishing features include memory encryption support and possibly branch prediction. This ISA will support:

| Feature | Description |
|---------|-------------|
| **Word Size** | 16-bit |
| **Breakpoint Support** | Allows stopping the program at specific points |
| **Instructions** | ADD, SUB, MUL, DIVMOD, AND, OR, XOR, LSH, LW, SW, BEQ, BLT, CMP, CALL, JMP, RET |
| **Encryption Instructions** | AESENC, AESDEC, AESKEY (Not included currently) |

### **Registers**

| Register | Description |
|----------|-------------|
| **R0**  | Always 0 |
| **R1**  | Always 1 |
| **R2-R12** | General Purpose |
| **R13** | Link Register (LR) (convention) |
| **R14** | Status Register (SR) (Handles comparison results) |
| **R15** | Program Counter (PC) |

---
## Simulation

We will develop a GUI supporting various functions and multiple display modes categorized as follows:

### **Controls (Inputs):**
| Control | Description |
|---------|-------------|
| Upload File | Uploads a program for execution |
| Run | Runs the entire program from start to finish |
| Step | Manually step through instructions |
| Breakpoint | Run until a specified instruction number |
| Run With | Run without cache or pipeline |
| Clear | Clears the specified memory (Cache, DRAM) |
| Delay | Set access delays for different pipeline stages |

### **Instructions Display (3 Tabs):**
| Tab | Description |
|-----|-------------|
| **Code View** | Lists source code of the input |
| **Instruction View** | Shows current text and binary instructions |
| **Pipeline View** | Displays the state of each part of the pipeline |

### **Memory Display (3 Tabs):**
| Tab | Description |
|-----|-------------|
| **Registers** | Shows the current data within registers |
| **Cache** | Displays current data inside the cache |
| **DRAM** | Displays current data inside DRAM |

The GUI updates when one of the following occurs:
1. User selects 'Run' and execution completes.
2. User takes a 'Step'.
3. User sets a Breakpoint and hits 'Run'.

The source code updates in the GUI upon file upload, allowing full control and a clear view of how the ISA and program execution work at a low level.

---
## Instruction Details

The instructions for ARCH-16 are categorized as follows:

### **RRR-Type (Register Operations)**
| OpCode | Instruction | Encoding | Description |
|--------|------------|----------|-------------|
| 0000 | ADD | Rd, Ra, Rb | Rd = Ra + Rb |
| 0001 | SUB | Rd, Ra, Rb | Rd = Ra - Rb |
| 1100 | MUL | Rd, Ra, Rb | Rd = Ra * Rb |
| 1101 | DIVMOD | Rd, Ra, Rb | Rd = Ra / Rb, Rb = Ra % Rb |
| 0010 | AND | Rd, Ra, Rb | Rd = Ra & Rb |
| 0011 | OR | Rd, Ra, Rb | Rd = Ra | Rb |
| 0100 | XOR | Rd, Ra, Rb | Rd = Ra ^ Rb |
| 1011 | CMP | Rd, Ra, Rb (ignored) | SR = Rd - Ra |

### **Control Flow Instructions**
| Instruction | Description |
|------------|-------------|
| CALL R12 | ADD R13, PC, R1; OR PC, R12, R0 |
| RET | OR R15, R13, R0 |
| JMP R12 | OR R15, R12, R0 |

### **RTRI-Type (Register-Type-Register Operations)**
| OpCode | Type | Source Register | Destination Register | Description |
|--------|------|----------------|----------------------|-------------|
| 0101 | Shift Type | Rd | Ra | LSL Rd, Ra → Rd = Rd << Ra |

Types: [LSL] : 00, [LSR] : 01, [ROL] : 10, [ROR] : 11

### **RRI-Type (Register-Register-Immediate Operations)**
| OpCode | Instruction | Encoding | Description |
|--------|------------|----------|-------------|
| 0110 | LW | Rd, [Ra + imm] | Load word |
| 0111 | SW | [Ra + imm], Rd | Store word |
| 1000 | BEQ | Rd, Ra, imm | Rd == Ra ? PC += imm : PC++ |
| 1111 | BLT | Rd, Ra, imm | Rd < Ra ? PC += imm : PC++ |

Note: BEQ/BLT is PC-relative, so the immediate is an offset to R15.

---
## Memory System

The ARCH-16 includes an L1 cache and DRAM with the following specifications:

### **Cache:**
| Feature | Specification |
|---------|---------------|
| Mapping | Direct-mapping & 2-way set-associative |
| Replacement Policy | Least Recently Used (LRU) |
| Write Policy | Write-through, no-allocate |
| Size | 64 words, line length of 4 words |
| Benchmarking | Planned for cache efficiency |

### **DRAM:**
| Feature | Specification |
|---------|---------------|
| Simulation | Models real-world latencies |
| Adjustable Memory | Supports encryption and extras |
| Size | 50K words |

---
## Project Approach

We will implement this project using **C** for simulation and **Qt or JavaFX** for the GUI. Development will be managed via **GitHub** (for documentation, code, and issues), and we will communicate through **weekly Friday meetings and Google Chat**.

### **Development Plan:**
1. **Complete Planning** - Finalize the system design.
2. **Memory System Implementation** - Develop the cache and DRAM.
3. **Basic UI Development** - Create an interface for ease of testing.
4. **Pipeline Implementation** - Develop and test each pipeline stage in sections.
5. **Instruction Implementation** - Define and implement the ISA instructions.
6. **Assembler Development** - Create an assembler for the ARCH-16 ISA.
7. **Full Pipeline Testing** - Ensure correct execution of sample programs.
8. **GUI Enhancements** - Improve UI functionality and polish visuals.
9. **Benchmark Testing** - Measure performance with test programs.
10. **Final Features (if time permits)** - Implement **AES encryption** and **branch prediction**.

AES encryption and branch prediction are optional features we will decide on within the next few weeks, depending on time availability.

