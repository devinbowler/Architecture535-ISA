#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>

typedef struct {
  uint16_t opcode;
  uint16_t   regD;
  uint16_t   regA;
  uint16_t   regB;
} RRRinstr;

typedef struct {
  uint16_t opcode;
  uint16_t   type;
  uint16_t   regA;
  uint16_t   regB;
} RRinstr;

typedef struct {
  uint16_t opcode;
  uint16_t   regD;
  uint16_t   regA;
  uint16_t    imm;
} RRIinstr;

uint16_t loadInstruction(const char *line);

uint16_t RRRTypeEncode(RRRinstr *instr);
uint16_t RRTypeEncode(RRinstr *instr);
uint16_t RRITypeEncode(RRIinstr *instr);

#endif
