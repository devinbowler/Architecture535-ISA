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
