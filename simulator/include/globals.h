#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define DATA_OFFSET 0

extern bool     memory_operation_in_progress;

extern uint16_t USER_DRAM_DELAY;
extern uint16_t USER_CACHE_DELAY;

extern bool     PIPELINE_ENABLED;
extern bool     CACHE_ENABLED;
extern uint16_t CACHE_MODE;

#endif
