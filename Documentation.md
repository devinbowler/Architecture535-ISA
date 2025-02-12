# ARCH CPU Simulator - Project Timeline

---

## Phase 1: Foundation & Documentation (Week 1)
**Goal:** Define all specifications & ensure clarity before writing any code.

### Tasks
- [ ] Finish **ISA Specifications Document** (Register layout, opcode formats, memory organization)
- [ ] Decide on **16-bit vs. 32-bit memory addressing**
- [ ] Define **instruction encoding (binary representation)**
- [ ] Create a **reference sheet** for all opcodes and instruction formats
- [ ] Plan **how memory will be structured** (stack, heap, MMIO regions)
- [ ] Assign **team responsibilities** (Devin & Owen)
  
---

## Phase 2: Memory & Registers (Week 2)
**Goal:** Set up the memory system & register file.

### Tasks
#### **Memory Implementation**
- [ ] Create a **RAM array** to simulate memory (`uint16_t memory[65536]`)
- [ ] Implement **load/store functions** for memory access
- [ ] Ensure **word-alignment for memory accesses**
- [ ] Implement **stack memory (growing downward)**

#### **Register Implementation**
- [ ] Define a **register file** (`uint16_t registers[8]`)
- [ ] Initialize **special registers** (`PC`, `SP`, `SR`)
- [ ] Create functions to **read/write registers**
  
---

## Phase 3: CPU Core - Basic Fetch/Decode/Execute (Week 3)
**Goal:** Implement the **core execution loop**.

### Tasks
#### **Instruction Fetch**
- [ ] Implement **instruction fetch from memory** (`fetch_instruction()`)

#### **Instruction Decode**
- [ ] Extract **opcode & register fields** (`decode_instruction()`)

#### **Instruction Execution**
- [ ] Implement **ADD instruction**
- [ ] Implement **SUB instruction**
- [ ] Implement **AND/OR logic operations**
- [ ] Implement **LW (Load Word)**
- [ ] Implement **SW (Store Word)**
- [ ] Implement **BEQ (Branch if Equal)**
- [ ] Implement **BNE (Branch if Not Equal)**
- [ ] Implement **JUMP instruction**
- [ ] Implement **HALT instruction**

#### **Execution Loop**
- [ ] Implement the **main CPU loop** (`while (running) { fetch → decode → execute }`)

---

## Phase 4: Implement Pipeline (Week 4)
**Goal:** Introduce **pipeline stages** for performance.

### Tasks
#### **Pipeline Stages**
- [ ] Implement **Fetch Stage (IF)**
- [ ] Implement **Decode Stage (ID)**
- [ ] Implement **Execute Stage (EX)**
- [ ] Implement **Memory Stage (MEM)**
- [ ] Implement **Writeback Stage (WB)**

#### **Pipeline Control**
- [ ] Implement **pipeline registers** (storing intermediate results)
- [ ] Implement **pipeline flushing on jumps/branches**
- [ ] Implement **basic forwarding (if needed)**

---

## Phase 5: Implement Cache (Week 5)
**Goal:** Implement a **basic direct-mapped cache**.

### Tasks
#### **Cache Structure**
- [ ] Define a **direct-mapped cache** (`struct CacheLine { tag, data, valid }`)
- [ ] Implement **cache read/write operations**
- [ ] Implement **cache hit/miss logic**
- [ ] Implement **write-through policy (no-allocate)**

#### **Cache Performance Tracking**
- [ ] Implement **cache performance counters** (hit/miss tracking)
- [ ] Compare **cache-enabled vs. no-cache execution times**

---

## Phase 6: Assembler Development (Week 6)
**Goal:** Convert **assembly code → binary machine code**.

### ✅ Tasks
#### **Instruction Encoding**
- [ ] Define **instruction formats in the assembler**
- [ ] Implement **binary encoding rules for each instruction**

#### **Assembler Implementation**
- [ ] Implement **ADD/SUB assembler translation**
- [ ] Implement **LW/SW assembler translation**
- [ ] Implement **Branch/Jump assembler translation**
- [ ] Implement **HALT assembler translation**
- [ ] Implement **assembler output to binary file**
  
---

## Phase 7: CPU Simulation UI (Week 7)
**Goal:** Create a **user interface** for debugging & visualization.

### Tasks
#### **UI Design**
- [ ] Display **register contents**
- [ ] Display **memory contents**
- [ ] Display **pipeline stage information**
- [ ] Display **cache hit/miss stats**

#### **Simulation Controls**
- [ ] Implement **single-step execution**
- [ ] Implement **run to breakpoint**
- [ ] Implement **execution logging (register/memory updates)**

---

## Phase 8: Final Testing & Benchmarking (Week 8)
**Goal:** Verify correctness using **benchmark programs**.

### Tasks
#### **Run Benchmark Programs**
- [ ] Implement **Exchange Sort (Sorting Algorithm)**
- [ ] Implement **Matrix Multiplication**
- [ ] Analyze **execution cycle counts (with/without cache & pipeline)**

#### **Final Debugging**
- [ ] Fix **pipeline hazards**
- [ ] Fix **branch mispredictions**
- [ ] Fix **cache consistency issues**

#### **Final Report & Presentation**
- [ ] Prepare **demo & slides**
- [ ] Write **final project report**
- [ ] Practice **team presentation**

---

## Final Deliverables
### **Project Completion Checklist**
- [ ] **Working CPU Simulator**
- [ ] **Assembler**
- [ ] **Cache Simulation**
- [ ] **Pipeline Execution**
- [ ] **Final Report**
- [ ] **Presentation**

---

## Team Responsibilities
### **Devin's Responsibilities**
- [ ] TBD  

### **Owen's Responsibilities**
- [ ] TBD  
