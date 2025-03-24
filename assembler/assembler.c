// This file will take in a file of assembly code, and turn it into our custom encodings for decode stage to read.
// ADD R1, R2, R3 -> Convert to : 0000 0001 0010 0011 -> Save to DRAM

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "assembler.h"


// Each function will take in its own type and make the binary encoding.
// This uses the bit operation of & to check the opcode to 00001111, this not
// only ensures this field is 4 bits, but also copys the code correctly. Then
// we shift those copied 4 bits to the top 4 bits of the encoded number.
uint16_t RRRTypeEncode(RRRinstr *instr){
 printf("Passed instruction: opcode=%u, regD=%u, regA=%u, regB=%u\n",
          instr->opcode, instr->regD, instr->regA, instr->regB);

 uint16_t encoded = 0;
 encoded |= (instr->opcode & 0xF) << 12; // top 4 bits  | Opcode
 encoded |= (instr->regD & 0xF) << 8;    // next 4 bits | Destination Regsiter
 encoded |= (instr->regA & 0xF) << 4;    // top 4 bits  | Source Register A
 encoded |= (instr->regB & 0xF);         // last 4 bits | Source Register B


 return encoded;
}

uint16_t RRTypeEncode(RRinstr *instr){
 printf("Passed instruction: opcode=%u, type=%u, regA=%u, regB=%u\n",
          instr->opcode, instr->type, instr->regA, instr->regB);

 uint16_t encoded = 0;
 encoded |= (instr->opcode & 0xF) << 12; // top 4 bits   | Opcode
 encoded |= (instr->type & 0xF) << 8;    // next 4 bits  | Type
 encoded |= (instr->regA & 0xF) << 4;    // top 4 bits   | Source Register A
 encoded |= (instr->regB & 0xF);         // last 4 bits  | Source Register B


 return encoded;
}

uint16_t RRITypeEncode(RRIinstr *instr){
 printf("Passed instruction: opcode=%u, regD=%u, regA=%u, imm=%u\n",
          instr->opcode, instr->regD, instr->regA, instr->imm);

 uint16_t encoded = 0;
 encoded |= (instr->opcode & 0xF) << 12; // top 4 bits  | Opcode
 encoded |= (instr->regD & 0xF) << 8;    // next 4 bits | Destination Regsiter
 encoded |= (instr->regA & 0xF) << 4;    // top 4 bits  | Source Register A
 encoded |= (instr->imm & 0xF);          // last 4 bits | Immediate


 return encoded;
}


// We will from UI call to this for each line, and directly save to DRAM from 
uint16_t loadInstruction(const char *line){
  // Get the opcode, assembly.
  RRRinstr rrr;
  RRinstr rr;
  RRIinstr rri;

  char lineCopy[128];
  strncpy(lineCopy, line, sizeof(lineCopy));
  lineCopy[sizeof(lineCopy)-1] = '\0';

  printf("Instruction Line: %s\n.", lineCopy);

  char *values[4];
  char *value = strtok(lineCopy, " ");
  uint16_t valueCount = 0;

  while (value != NULL && valueCount < 4) {
    values[valueCount++] = value;
    value = strtok(NULL, " ");
  }

  if (valueCount == 4){
    char *opcode = values[0];
    uint16_t rd = atoi(values[1] + 1);
    uint16_t ra = atoi(values[2] + 1);
    uint16_t rb = atoi(values[3] + 1);

    if (strcmp(opcode, "ADD") == 0){        // RRR Types
      rrr.opcode = 0000;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "SUB") == 0){
      rrr.opcode = 0001;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "AND") == 0){
      rrr.opcode = 0010;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "OR") == 0) {
      rrr.opcode = 0011;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "XOR") == 0){
      rrr.opcode = 0100;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "DIVMOD") == 0){
      rrr.opcode = 0101;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "MUL") == 0){ 
      rrr.opcode = 0110;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "CMP") == 0){
      rrr.opcode = 0111;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "LSL") == 0){ // RR Types
      rr.opcode = 1000;
      rr.type = 0000;
      rr.regA = ra;
      rr.regB = rb;

      return RRTypeEncode(&rr);
    } else if (strcmp(opcode, "LSR") == 0){
      rr.opcode = 1000;
      rr.type = 0001;
      rr.regA = ra;
      rr.regB = rb;

      return RRTypeEncode(&rr);
    } else if (strcmp(opcode, "ROL") == 0){
      rr.opcode = 1000;
      rr.type = 0010;
      rr.regA = ra;
      rr.regB = rb;

      return RRTypeEncode(&rr);
    } else if (strcmp(opcode, "ROR") == 0){
      rr.opcode = 1000;
      rr.type = 0011;
      rr.regA = ra;
      rr.regB = rb;

      return RRTypeEncode(&rr);
    } else if (strcmp(opcode, "LW") == 0) { // RRI Types
      rri.opcode = 1001;
      rri.regD = rd;
      rri.regA = ra;
      rri.imm = rb;

      return RRITypeEncode(&rri);
    } else if (strcmp(opcode, "SW") == 0) {
      rri.opcode = 1010;
      rri.regD = rd;
      rri.regA = ra;
      rri.imm = rb;

      return RRITypeEncode(&rri);
    } else if (strcmp(opcode, "BEQ") == 0){
      rri.opcode = 1011;
      rri.regD = rd;
      rri.regA = ra;
      rri.imm = rb;

      return RRITypeEncode(&rri);
    }
  } else {
    printf("Invalid instruction, [ %s ].\n", line);
  }
}


int main() {
  const char *line = "LW R3 R2 0001";
  uint16_t encoded = loadInstruction(line);
  printf("Encoded Instruction: 0x%04X\n", encoded);
  return 0;
}
