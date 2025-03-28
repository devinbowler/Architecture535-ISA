typedef struct {
    Instruction fetch;
    Instruction decode;
    Instruction execute;
    Instruction mem;
    Instruction writeback;
} PipelineState;

void pipeline_step();
