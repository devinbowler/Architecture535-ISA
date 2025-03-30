#include "memory_access.h"

/**
 * @brief Implements the memory access stage of the pipeline. This will just use our existing cache and DRAM functions
 * @param pipeline the pipeline
 */
void memory_access(PipelineState *pipeline) {

}

bool memory_ready(PipelineState *pipeline) {
  return pipeline->MEM_WB.valid;
}
