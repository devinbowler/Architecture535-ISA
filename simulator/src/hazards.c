#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "hazards.h"
#include "pipeline.h"
#include "globals.h"


bool     data_hazard_stall      = false;
uint16_t stall_cycles_remaining = 0;


/* helper: does this opcode write to a register? */
static inline bool writes_reg(uint16_t op){
    /* 0-6  : ALU ops -- write
       7-8  : store/branch -- no write
       9    : LW -- write
       10-? : mul/div/etc → assume write                              */
    return (op <= 6) || op == 9 || op >= 10;
}

/* helper: does the current instruction *use* rt (regB) as a source?  */
static inline bool uses_regB(uint16_t op){
    /* immediate / unary ops would not use rt; here only 0-2 (ADD/SUB/MUL)
       and 11 (CMP) need rt                                            */
    return (op <= 2) || op == 11;
}


HazardInfo detect_hazards(PipelineState *p)
{
    HazardInfo hz = {false,false,0,0,0,0,0};

    if (!p->ID_EX.valid) return hz;          /* nothing to check */

    uint16_t rs = p->ID_EX.regA;             /* source regs of instr in Decode */
    uint16_t rt = p->ID_EX.regB;

    /* -------- check EX/MEM stage (just executed) ------------------- */
    if (p->EX_MEM.valid && writes_reg(p->EX_MEM.opcode)){
        uint16_t rd = p->EX_MEM.regD;

        if (rd != 0 && (rd == rs || (uses_regB(p->ID_EX.opcode) && rd == rt))){
            hz.detected     = true;
            hz.source_reg   = rd;
            hz.target_reg   = (rd == rs) ? rs : rt;
            hz.source_stage = 1;                 /* EX/MEM */

            if (p->EX_MEM.opcode == 9){          /* load-use hazard */
                hz.requires_stall = true;
                hz.stall_cycles   = 1;
            }else{
                hz.forwarded_value = p->EX_MEM.res;
            }
            return hz;
        }
    }

    /* -------- check MEM/WB stage (about to write back) -------------- */
    if (p->MEM_WB.valid && writes_reg(p->MEM_WB.opcode)){
        uint16_t rd = p->MEM_WB.regD;

        if (rd != 0 && (rd == rs || (uses_regB(p->ID_EX.opcode) && rd == rt))){
            hz.detected         = true;
            hz.source_reg       = rd;
            hz.target_reg       = (rd == rs) ? rs : rt;
            hz.source_stage     = 2;                 /* MEM/WB */
            hz.forwarded_value  = p->MEM_WB.res;
        }
    }
    return hz;
}


void resolve_hazards(PipelineState *p, HazardInfo *hz)
{
    if (!hz->detected) return;

    /* need to stall? (load-use) */
    if (hz->requires_stall){
        data_hazard_stall      = true;
        stall_cycles_remaining = hz->stall_cycles;
        printf("[HAZARD] load-use, stalling %u cycle(s)\n", hz->stall_cycles);
        return;
    }

    /* otherwise forward right into the register file for EXEC stage   */
    forward_result(p, hz);
}

void forward_result(PipelineState *p, HazardInfo *hz)
{
    if (!hz->detected || hz->requires_stall) return;

    extern REGISTERS *registers;   /* from globals */

    uint16_t reg = hz->target_reg;
    uint16_t old = registers->R[reg];
    registers->R[reg] = hz->forwarded_value;

    printf("[FORWARD] R%u: %u → %u (from stage %u)\n",
           reg, old, hz->forwarded_value, hz->source_stage);
}
