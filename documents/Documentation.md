# ARCH CPU Simulator - Project Timeline (ISA First Approach)

---

## Phase 1: Design the Instruction Set
Goal: Define the full ISA before writing any code.

### Tasks
- [ ] Write out all instructions (ADD, SUB, LW, etc.), ensuring a minimal yet functional instruction set.
- [ ] Define binary encoding format for each instruction.
  - Example:
    - `ADD R1, R2` → `0000 001 010 0000`
    - `LW R1, [0x1000]` → `0100 001 000000000100000`
- [ ] Decide register layout (How many? What do they do?).
  - Example:
    - `R0`: Always zero
    - `R1`: Function return value
    - `R6`: Stack pointer (SP)
    - `R7`: Status register (SR) for flags
- [ ] Decide memory organization (Stack, Heap, ROM, MMIO).
  - Example:
    - `0x0000 - 0x3FFF`: ROM (read-only program code)
    - `0x4000 - 0x7FFF`: General RAM
    - `0x9000 - 0x9FFF`: Stack memory (grows downward)
- [ ] Write an ISA Specification Document for reference.

Deliverable: A document that describes the entire ISA, including:
1. List of instructions.
2. Binary encoding format.
3. Register definitions and function.
4. Memory layout and stack behavior.

---

## Phase 2: Implement the Assembler
Goal: Create an assembler that converts human-readable instructions into binary machine code.

### Tasks
- [ ] Implement opcode translation (convert ADD, SUB, LW, etc., to binary).
- [ ] Implement register translation (R1 → `001`, R2 → `010`, etc.).
- [ ] Implement memory addressing translation for immediate values.
  - Example:
    ```
    LW R1, [0x1000]  
    ```
    Converts to:
    ```
    0100 001 000000000100000
    ```
- [ ] Output binary machine code files that the simulator can run.

Deliverable: A working assembler that can take: ADD R1, R2. And convert it into: 000000101000

---

## Phase 3: Implement CPU Core
Goal: Create the basic CPU execution loop to process instructions.

### Tasks
- [ ] Implement registers in software (`uint16_t registers[8]`).
- [ ] Implement instruction fetch mechanism (`fetch_instruction()`).
- [ ] Implement instruction decode mechanism (`decode_instruction()`).
- [ ] Implement basic execution loop (`while (running) { fetch → decode → execute }`).
- [ ] Implement a simple instruction execution function.
  - Example:
    ```cpp
    switch (opcode) {
        case 0x0: // ADD
            registers[rd] = registers[rd] + registers[rs];
            break;
        case 0x4: // LW
            registers[rd] = memory[address];
            break;
    }
    ```

Deliverable: A CPU that can fetch, decode, and execute ADD/SUB.

---

## Phase 4: Implement Memory System
Goal: Add memory handling to support load/store instructions.

### Tasks
- [ ] Implement a RAM array (`uint16_t memory[65536]` to simulate 64KB memory).
- [ ] Implement load-word (LW) instruction.
- [ ] Implement store-word (SW) instruction.
- [ ] Implement stack pointer (SP) and function call handling.
  - Example:
    ```cpp
    void push(uint16_t value) {
        memory[sp--] = value;
    }

    uint16_t pop() {
        return memory[++sp];
    }
    ```

Deliverable: A CPU that can load and store data in memory.

---

## Phase 5: Implement Pipeline
Goal: Introduce pipelining for parallel execution.

### Tasks
- [ ] Implement Fetch → Decode → Execute → Memory → Writeback stages.
- [ ] Implement pipeline registers for each stage.
- [ ] Implement flush mechanism for jumps/branches.
- [ ] Implement data forwarding to reduce pipeline hazards.
  - Example:
    ```
    ADD R1, R2
    SUB R3, R1  // Needs R1 from previous instruction, potential hazard
    ```

Deliverable: A pipelined CPU that executes multiple instructions in parallel.

---

## Phase 6: Implement Cache
Goal: Add a cache layer to optimize memory access.

### Tasks
- [ ] Implement a direct-mapped cache structure.
  - Example:
    ```cpp
    struct CacheLine {
        uint16_t tag;
        uint16_t data;
        bool valid;
    };
    ```
- [ ] Implement hit/miss logic.
  - Example:
    ```cpp
    if (cache[index].valid && cache[index].tag == address >> 4) {
        return cache[index].data; // Cache hit
    } else {
        cache[index].data = memory[address]; // Cache miss, load from memory
        cache[index].tag = address >> 4;
        cache[index].valid = true;
    }
    ```
- [ ] Implement write-through policy with no-allocate on writes.
- [ ] Implement cache performance counters (hit/miss tracking).

Deliverable: A cache that speeds up CPU execution.

---

## Phase 7: Implement User Interface
Goal: Build a UI that provides visibility into CPU execution.

### Tasks
- [ ] Implement a **register viewer** that displays CPU register values in hexadecimal.
- [ ] Implement a **memory viewer** that allows scrolling through RAM contents.
- [ ] Implement a **pipeline visualization tool** to show instruction progress.
- [ ] Implement a **cache viewer** to monitor cache hit/miss rates.
- [ ] Implement step execution:
  - Load binary programs and execute one instruction at a time.
  - Show changes in registers and memory after each instruction.
- [ ] Implement a mode selector:
  - No pipeline, no cache.
  - Pipeline only.
  - Cache only.
  - Full pipeline and cache enabled.
- [ ] Implement program loading and execution controls.
- [ ] Implement execution cycle count tracking.

Deliverable: A UI that allows users to:
1. Step through instructions and observe execution.
2. Toggle pipeline and cache settings.
3. View CPU state, memory, and execution statistics.

---

## Phase 8: Testing and Benchmarking
Goal: Validate performance using sorting and matrix multiplication.

### Tasks
- [ ] Implement sorting algorithm in assembly.
  - Example (Exchange Sort):
    ```
    LW R1, [R2]
    LW R3, [R4]
    BNE R1, R3, SWAP
    ADD R2, R2, 2
    ADD R4, R4, 2
    J LOOP
    SWAP:
    SW [R2], R3
    SW [R4], R1
    ```
- [ ] Implement matrix multiplication in assembly.
- [ ] Measure cycle counts with/without cache and pipeline.

Deliverable: A final report with performance results.

---

## Phase 9: Final Debugging and Presentation
Goal: Finalize the project, debug errors, and prepare a presentation.

### Tasks
- [ ] Fix bugs in the pipeline and cache.
- [ ] Create demo programs to showcase CPU execution.
- [ ] Prepare presentation slides explaining:
  - ISA design choices.
  - How the CPU executes instructions.
  - Pipeline optimizations and cache performance.
- [ ] Write the final project report.

Deliverable: A fully working CPU simulator with test programs.

---

## Final Deliverables
- [ ] ISA Specification Document
- [ ] Working CPU Simulator
- [ ] Assembler
- [ ] Cache Simulation
- [ ] Pipeline Execution
- [ ] User Interface for CPU Monitoring
- [ ] Final Report
- [ ] Presentation
