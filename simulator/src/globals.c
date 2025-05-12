/* globals.c – central home for run‑time flags */
#include <stdint.h>
#include <stdbool.h>

bool     memory_operation_in_progress = false;

uint16_t USER_DRAM_DELAY   = 4;      /* default 4  cycles */
uint16_t USER_CACHE_DELAY  = 1;      /* default 1  cycle  */

bool     PIPELINE_ENABLED  = true;   /* “Pipeline Enabled” check‑box  */
bool     CACHE_ENABLED     = true;   /* “Cache Enabled”    check‑box  */
uint16_t CACHE_MODE        = 2;      /*Set Associative*/