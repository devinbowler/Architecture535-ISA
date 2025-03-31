#include "write_back.h"

/**
 * @brief Implements the write back stage of the pipeline. This will also just use our memory functions
 * @param pipeline the pipeline
 */
void write_back(PipelineState *pipeline) {

}

/**
 * @brief 
 * @param pipeline the pipeline
 * @return true if the write back stage is ready
 * @return false if the write back stage is not ready
 */
bool write_back_ready(PipelineState *pipeline) {
  return pipeline->WB.valid;
}
