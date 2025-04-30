#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "pipeline.h"
#include "memory.h"
#include "globals.h"
#include "fetch.h"
#include "decode.h"
#include "execute.h"
#include "memory_access.h"
#include "write_back.h"
#include "hazards.h"    //  <— new: centralised RAW / load‑use detection

extern bool data_hazard_stall;           // set by resolve_hazards() when a stall is required
extern uint16_t stall_cycles_remaining;  // countdown handled right here each cycle
extern REGISTERS *registers;             // architectural register file (for bypassing etc.)
extern bool branch_taken;                // asserted by execute() when branch target chosen
extern bool memory_operation_in_progress;// long‑latency memory op (not cache) in MEM stage
extern bool fetch_memory_busy;           // IF stage is currently waiting on ICACHE miss

void pipeline_step(PipelineState *p, uint16_t *value)
{
    // 1) Commit tail stages first (WB → MEM)
    write_back(p);
    memory_access(p);

    // 2) Hazrd Detection
    HazardInfo h = detect_hazards(p);          // consult ID/EX + EX/MEM + MEM/WB

    if (h.detected) {
        resolve_hazards(p, &h);                // may set data_hazard_stall & countdown
    }

    // 3) Stall Logic
    // (priority: memory busy  >  explicit RAW stall  >  normal advance)

    if (memory_operation_in_progress) {
        // Freeze everything *except* MEM/WB & WB so the long latency op can retire.
        p->WB     = p->WB_next;
        p->MEM_WB = p->MEM_WB_next;
        printf("[PIPELINE_STALL] Memory op in progress → stalling IF/ID, ID/EX, EX/MEM\n");
    }
    else if (data_hazard_stall) {
        // Inject bubble at EX/MEM, hold earlier latches.  (Classic load‑use solution)
        p->EX_MEM.valid = false;          // bubble
        p->WB     = p->WB_next;
        p->MEM_WB = p->MEM_WB_next;

        if (stall_cycles_remaining) {
            --stall_cycles_remaining;
        }
        if (stall_cycles_remaining == 0) {
            data_hazard_stall = false;    // stall window has elapsed – resume next cycle
        }
    }
    else {
        // 4) Normal Advance
        execute(p);

        if (PIPELINE_ENABLED) {
            // five‑stage parallel flow
            decode_stage(p);
            fetch_stage(p, value);   // handles its own icache penalties via fetch_memory_busy
        } else {
            // single‑issue / non‑pipelined debug mode
            bool empty = !p->ID_EX.valid && !p->EX_MEM.valid && !p->MEM_WB.valid;

            if (empty) {
                if (p->IF_ID.valid) {
                    decode_stage(p);
                    printf("[PIPELINE] Non‑pipe: decoding\n");
                } else {
                    fetch_stage(p, value);
                    printf("[PIPELINE] Non‑pipe: fetching\n");
                }
            } else {
                p->ID_EX_next.valid = false;
                p->IF_ID_next.valid = false;
                printf("[PIPELINE] Non‑pipe: draining\n");
            }
        }

        // 5) Commit next state
        p->WB     = p->WB_next;
        p->MEM_WB = p->MEM_WB_next;
        p->EX_MEM = p->EX_MEM_next;
        p->ID_EX  = p->ID_EX_next;
        p->IF_ID  = p->IF_ID_next;
    }

    // 6) Zero out
    memset(&p->WB_next,     0, sizeof p->WB_next);
    memset(&p->MEM_WB_next, 0, sizeof p->MEM_WB_next);
    memset(&p->EX_MEM_next, 0, sizeof p->EX_MEM_next);
    memset(&p->ID_EX_next,  0, sizeof p->ID_EX_next);
    memset(&p->IF_ID_next,  0, sizeof p->IF_ID_next);
}

// Called by execute() the cycle a branch is resolved & taken.  We squash only the
// younger (earlier‑stage) instructions – not the branch itself.
void mark_subsequent_instructions_as_squashed(PipelineState *p)
{
    if (p->IF_ID.valid) {
        p->IF_ID.squashed = true;
        printf("[BRANCH] Squashing IF/ID @ PC=%u\n", p->IF_ID.pc);
    }
    if (p->ID_EX.valid) {
        p->ID_EX.squashed = true;
        printf("[BRANCH] Squashing ID/EX @ PC=%u\n", p->ID_EX.pc);
    }
}
