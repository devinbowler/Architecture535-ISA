// This file will take in a file of assembly code, and turn it into our custom encodings for decode stage to read.
// ADD R1, R2, R3 -> Convert to : 0000 0001 0010 0011 -> Save to DRAM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint16_t RRRTypeEncode(RRRinstr *instr){
}

uint16_t RRTypeEncode(RRinstr *instr){
}

uint16_t RRITypeEncode(RRIinstr *instr){
}


// We will from UI call to this for each line, and directly save to DRAM from 
uint16_t loadInstruction(const char *file){
  // Get the opcode, assembly.
  RRRinstr rrr;
  RRinstr rr;
  RRIinstr rri;

  char opcode[] = "ADD";

  if (strcmp(opcode, "ADD") == 0){        // RRR Types
    rrr.opcode = 0;
    rrr.regD = 1;
    rrr.regA = 2;
    rrr.regB = 3;
    return RRRTypeEncode(&rrr);
  } else if (strcmp(opcode, "SUB") == 0){
  } else if (strcmp(opcode, "AND") == 0){
  } else if (strcmp(opcode, "OR") == 0) {
  } else if (strcmp(opcode, "XOR") == 0){
  } else if (strcmp(opcode, "CPY") == 0){
  } else if (strcmp(opcode, "CMP") == 0){ // RR Types
  } else if (strcmp(opcode, "LSH") == 0){
  } else if (strcmp(opcode, "ROT") == 0){
  } else if (strcmp(opcode, "LW") == 0) { // RRI Types
  } else if (strcmp(opcode, "SW") == 0) {
  } else if (strcmp(opcode, "BEQ") == 0){
  }
} 

