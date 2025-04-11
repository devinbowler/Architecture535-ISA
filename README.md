# ARCH-16 Simulator

ARCH‑16 is a custom 16‑bit general purpose Instruction Set Architecture (ISA) developed as part of Project 535. It features a five‑stage pipeline simulator, supports memory encryption (with plans for AES instructions) and may eventually include branch prediction. This repository contains the core simulator (written in C), an assembler that encodes the instructions, a Flask-based API for interfacing with the simulator, and a GUI for visualizing and controlling simulation states.

## Authors
- **Devin Bowler**
- **Owen Gibbons**

## Table of Contents
- [Overview](#overview)
- [General Features](#general-features)
- [Instruction Set Architecture](#instruction-set-architecture)
  - [RRR-Type Instructions](#rrr-type-instructions)
  - [RTRI-Type (Register-Register-Type) Instructions](#rtri-type-instructions)
  - [RRI-Type (Register-Register-Immediate) Instructions](#rri-type-instructions)
- [Simulation & Pipeline](#simulation--pipeline)
- [Memory System](#memory-system)
- [GUI and API](#gui-and-api)
- [Project Approach](#project-approach)
- [Building and Running](#building-and-running)
- [Contribution & Issues](#contribution--issues)

## Overview

ARCH‑16 is designed as a general purpose 16‑bit processor architecture. It uses a five‑stage pipeline—including fetch, decode, execute, memory, and write‑back—to simulate instruction processing. The simulator supports memory encryption and, as future enhancements, branch prediction. The system is interfaced via a RESTful API that communicates with a GUI, enabling you to load, run, and step through instructions while visualizing the pipeline and memory states.

## General Features

- **Word Size:** 16 bits
- **Registers:** 16 registers with the following specifications:
  - **R0:** Always 0
  - **R1:** Always 1
  - **R2-R12:** General Purpose Registers
  - **R13:** Link Register (LR)
  - **R14:** Status Register (SR) – handles comparison results
  - **R15:** Program Counter (PC)
- **Breakpoint Support:** Stop execution at a specified instruction.
- **Instructions Supported:**
  - **ADD Rd, Ra, Rb**
  - **SUB Rd, Ra, Rb**
  - **MUL Rd, Ra, Rb**
  - **DIVMOD Rd, Ra, Rb**
  - **AND Rd, Ra, Rb**
  - **OR Rd, Ra, Rb**
  - **XOR Rd, Ra, Rb**
  - **LSH Rd, Ra, Rb** (allows shifting by a register value)
  - **LW Rd, [Ra + imm]**
  - **SW [Ra + imm], Rd**
  - **BEQ Rd, Ra, imm**
  - **BLT Rd, Ra, imm**
  - **CMP Rd, Ra, Rb**
- **Encryption Instructions (Planned):**
  - AESENC
  - AESDEC
  - AESKEY

## Instruction Set Architecture

### RRR-Type Instructions (Register-Register-Register)
- **Format:**  
  - OpCode (4 bits)  
  - Register D (4 bits)  
  - Register A (4 bits)  
  - Register B (4 bits)
- **Examples:**
  - **ADD:** `0000 R[dst], R[srcA], R[srcB]`  → Rd = Ra + Rb  
  - **SUB:** `0001 R[dst], R[srcA], R[srcB]`  → Rd = Ra - Rb  
  - **AND:** `0010 R[dst], R[srcA], R[srcB]`  → Rd = Ra & Rb  
  - **OR:**  `0011 R[dst], R[srcA], R[srcB]`  → Rd = Ra | Rb  
  - **XOR:** `0100 R[dst], R[srcA], R[srcB]`  → Rd = Ra xor Rb  
  - **DIVMOD:** `0101 R[dst], R[srcA], R[srcB]`  → Rd = Ra / Rb, and Rb = Ra % Rb  
  - **MUL:** `0110 R[dst], R[srcA], R[srcB]`  → Rd = Ra * Rb  
  - **CMP:** `0111 R[dst], R[srcA], R[srcB]`  → SR = Rd - Ra (R[srcB] ignored)  

*Note:* CALL, JMP, and RET are implemented using a combination of ADD and OR instructions.

### RTRI-Type Instructions (Register-Register-Type)
- **Format:**  
  - OpCode (4 bits)  
  - Type (2 bits)  
  - Source Register (4 bits)  
  - Destination Register (4 bits)
- **Example for Shifts and Rotations:**
  - **LSH/ROT:**  
    - `1000` with type bits:
      - `00` for LSL  
      - `01` for LSR  
      - `10` for ROL  
      - `11` for ROR  
    - Format: `LSH Rd, Ra`  → Rd is shifted/rotated by the value in Ra.

### RRI-Type Instructions (Register-Register-Immediate)
- **Format:**  
  - OpCode (4 bits)  
  - Register D (4 bits)  
  - Register A (4 bits)  
  - Immediate (4 bits)
- **Examples:**
  - **LW:** `1001 R[dst], [R[src] + imm]`  → Load from memory  
  - **SW:** `1010 [R[src] + imm], R[src2]` → Store to memory  
  - **BEQ:** `1011 R[dst], R[src], imm`     → Branch if R[dst] equals R[src]  
  - **BLT:** `1111 R[dst], R[src], imm`     → Branch if R[dst] is less than R[src]

*Note:* BEQ and BLT are PC-relative instructions; the immediate serves as an offset added to R15.

## Simulation & Pipeline

The simulator implements a five-stage pipeline:
1. **Fetch**
2. **Decode**
3. **Execute**
4. **Memory Access**
5. **Write-back**

Each pipeline stage communicates its results using `printf` (which outputs to stdout). The API listens for these messages and extracts key state information (e.g., register values, memory and cache contents, and pipeline stage summaries).

A special breakpoint command and step-by-step execution are provided. The simulator also supports an API that allows:
- **Run** – Execute the complete program from start to finish.
- **Step** – Execute one pipeline cycle at a time.
- **Breakpoint** – Execute until a specified instruction number is reached.
- **Reset** – Reset the simulator state.

## Memory System

ARCH‑16 has two levels of memory:
- **Cache:** Supports direct-mapped (and optionally two‑way set associative) configurations using a write‑through, no‑allocate policy. The cache is 64 words in size with a block (line) length of 4 words.
- **DRAM:** Simulated with real‑world delay logic and adjustable access delays.

## GUI and API

A graphical user interface (GUI) is provided (using PyQT) with the following display features:

### Controls (Inputs):
- **File Upload:** Load assembly code.
- **Run:** Execute the entire program.
- **Step:** Execute one pipeline cycle at a time.
- **Breakpoint:** Execute until a specified instruction.
- **Clear:** Reset memory.
- **Delay Control:** Adjust delays for cache and DRAM access.

### Display Tabs:
- **Code View:** Shows the original source code.
- **Instruction View:** Displays text and binary representations of instructions.
- **Pipeline View:** Displays the state of each pipeline stage.
- **Registers, Cache, and DRAM:** Separate tabs show the current register values, cache contents, and DRAM data.

The API (Flask-based) launches the simulator as a subprocess, writes commands to its stdin, and reads its stdout until an "[END]" marker is detected. The output is then parsed and sent as JSON to the GUI.

## Project Approach

Our overall approach is as follows:
- **Planning Phase:** Finish detailed planning, then break the project into major components.
- **Memory System Implementation:** Build and test the memory system (DRAM and Cache).
- **Pipeline Development:** Divide and conquer the pipeline stages (fetch, decode, execute, memory, write-back).
- **Instruction Set and Assembler:** Define each instruction’s behavior and encoding; build the assembler.
- **GUI Implementation:** Develop a GUI to visualize pipeline stages, memory, and registers.
- **Future Enhancements:** Implement AES encryption instructions and possibly branch prediction.

Our communication includes weekly meetings (on Fridays) along with continuous communication through Google Chat. All progress and documentation are managed via GitHub using issues and pull requests.
### Building and Running

#### Building the Simulator
Compile the C simulator using your preferred build system.

**You can either run the MAKEFILE ('make', in the simulator directory), or the following gcc lines:**

### Linux - 
```bash
gcc simulator.c memory.c pipeline.c ./pipeline/fetch.c ./pipeline/decode.c ./pipeline/execute.c ./pipeline/memory_access.c ./pipeline/write_back.c ../assembler/assembler.c -o simulator
```

### Windows - 
```bash
gcc simulator.c memory.c pipeline.c ./pipeline/fetch.c ./pipeline/decode.c ./pipeline/execute.c ./pipeline/memory_access.c ./pipeline/write_back.c ../assembler/assembler.c -o simulator.exe
```

- Change the line in this code block,
```python
simulator_process = subprocess.Popen(
    ["../simulator/simulator"], # This line.
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True,
    bufsize=1
)
```
to
```python
    ["../simulator/simulator.exe"],
```

## Running the API

- Start the Flask API (which launches the simulator as a subprocess):

```bash
python3 api.py
```

The API will run on http://127.0.0.1:5000.

## Running the GUI

- After the API is running, start your GUI application:

```bash
python3 interface.py
```

Contribution & Issues

Contributions are welcome! Please open an issue or submit a pull request for any bug fixes or enhancements. All progress and documentation are managed via GitHub using issues and pull requests.
