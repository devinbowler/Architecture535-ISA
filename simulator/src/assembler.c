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

  char lineCopy[256]; // Increased buffer size
  strncpy(lineCopy, line, sizeof(lineCopy));
  lineCopy[sizeof(lineCopy)-1] = '\0';

  // I want to remove the (so allow) comments within files.
  char *comment = strchr(lineCopy, ';');
  if (comment != NULL){
    *comment = '\0'; 
  }

  // Also trim any white space.
  int len = strlen(lineCopy);
  while (len > 0 && (lineCopy[len-1] == ' ' || lineCopy[len-1] == '\t')){
    lineCopy[--len] = '\0';
  }

  // Also skip any empty lines or lines that have only comments
  if (len == 0) {
    return 0;
  }

  printf("Instruction Line: %s.\n", lineCopy);

  // First, replace commas with spaces for consistent parsing
  for (int i = 0; lineCopy[i]; i++) {
    if (lineCopy[i] == ',') {
      lineCopy[i] = ' ';
    }
  }
  
  // Special handling for LW and SW with [Ra + imm] syntax
  char *lw_pattern = strstr(lineCopy, "LW ");
  char *sw_pattern = strstr(lineCopy, "SW ");
  
  if (lw_pattern) {
    // Improved parsing for LW instruction
    char reg_dest[16], reg_base[16], offset_str[32];
    int rd = 0, ra = 0, imm = 0;
    
    // Extract the three main parts of the LW instruction
    char *bracket_start = strchr(lineCopy, '[');
    char *bracket_end = strchr(lineCopy, ']');
    
    if (bracket_start && bracket_end && bracket_start < bracket_end) {
      // Extract destination register
      if (sscanf(lineCopy, "LW R%d", &rd) == 1) {
        // Extract base register and offset
        char bracket_content[128];
        strncpy(bracket_content, bracket_start + 1, bracket_end - bracket_start - 1);
        bracket_content[bracket_end - bracket_start - 1] = '\0';
        
        // Check for R0 + offset pattern
        if (sscanf(bracket_content, "R%d + %d", &ra, &imm) == 2) {
          rri.opcode = 0b1001;
          rri.regD = rd;
          rri.regA = ra;
          rri.imm = imm & 0xF; // Limit to 4 bits but allow parsing larger values
          
          printf("Parsing LW: LW R%d, [R%d + %d]\n", rd, ra, imm);
          printf("Encoded as: destination=R%u, base=R%u, offset=%u (truncated to %u for encoding)\n", 
                 rd, ra, imm, rri.imm);
          return RRITypeEncode(&rri);
        }
      }
    }
  }
  else if (sw_pattern) {
    // Improved parsing for SW instruction
    char reg_src[16], reg_base[16], offset_str[32];
    int rd = 0, ra = 0, imm = 0;
    
    // Extract the three main parts of the SW instruction
    char *bracket_start = strchr(lineCopy, '[');
    char *bracket_end = strchr(lineCopy, ']');
    char *last_r = strrchr(lineCopy, 'R');
    
    if (bracket_start && bracket_end && bracket_start < bracket_end && last_r && last_r > bracket_end) {
      // Extract source register (after the closing bracket)
      if (sscanf(last_r, "R%d", &rd) == 1) {
        // Extract base register and offset
        char bracket_content[128];
        strncpy(bracket_content, bracket_start + 1, bracket_end - bracket_start - 1);
        bracket_content[bracket_end - bracket_start - 1] = '\0';
        
        // Check for R0 + offset pattern
        if (sscanf(bracket_content, "R%d + %d", &ra, &imm) == 2) {
          rri.opcode = 0b1010;
          rri.regD = rd;
          rri.regA = ra;
          rri.imm = imm & 0xF; // Limit to 4 bits but allow parsing larger values
          
          printf("Parsing SW: SW [R%d + %d], R%d\n", ra, imm, rd);
          printf("Encoded as: source=R%u, base=R%u, offset=%u (truncated to %u for encoding)\n", 
                 rd, ra, imm, rri.imm);
          return RRITypeEncode(&rri);
        }
      }
    }
  }

  // Standard parsing for other instructions
  char *values[4];
  char *value = strtok(lineCopy, " ");
  uint16_t valueCount = 0;

  while (value != NULL && valueCount < 4) {
    // Skip any extra spaces created by the comma replacement
    if (value[0] != '\0') {
      values[valueCount++] = value;
    }
    value = strtok(NULL, " ");
  }

  if (valueCount >= 1) {
    char *opcode = values[0];
    
    // Handle JMP instruction (no registers, just 12-bit immediate)
    if (strcmp(opcode, "JMP") == 0) {
      uint16_t imm12 = 0;
      
      // Parse the immediate value
      if (valueCount >= 2) {
        imm12 = atoi(values[1]) & 0xFFF;  // Limit to 12 bits
      }
      
      // Encode as: 0xC (opcode) + 12-bit immediate
      uint16_t encoded = 0;
      encoded |= (0xC & 0xF) << 12;  // opcode = 0xC (JMP)
      encoded |= (imm12 & 0xFFF);    // 12-bit immediate
      
      printf("Parsing JMP: JMP %d\n", imm12);
      printf("Encoded as: opcode=0xC, immediate=%u\n", imm12);
      
      return encoded;
    }
    
    // Improved register parsing with proper error handling
    uint16_t rd = 0, ra = 0, rb = 0, type = 0;
    
    // Parse register values for different instruction formats
    if (valueCount >= 3) {
      // Three-register format: OP Rd, Ra, Rb
      if (values[1][0] == 'R') rd = atoi(values[1] + 1); else rd = atoi(values[1]);
      if (values[2][0] == 'R') ra = atoi(values[2] + 1); else ra = atoi(values[2]);
      if (valueCount >= 4) {
        if (values[3][0] == 'R') rb = atoi(values[3] + 1); else rb = atoi(values[3]);
      }
      
      // LSH format or truncated three-register format
      if (strcmp(opcode, "LSH") == 0) {
        type = atoi(values[1]);
        if (values[2][0] == 'R') ra = atoi(values[2] + 1); else ra = atoi(values[2]);
        if (valueCount >= 4 && values[3][0] == 'R') rb = atoi(values[3] + 1); 
        else if (valueCount >= 4) rb = atoi(values[3]);
      }
    }

    // Standard ALU operations
    if (strcmp(opcode, "ADD") == 0){        // RRR Types
      rrr.opcode = 0b0000;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;
      
      printf("Parsing ADD: ADD R%d, R%d, R%d\n", rd, ra, rb);
      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "SUB") == 0){
      rrr.opcode = 0b0001;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;
      
      printf("Parsing SUB: SUB R%d, R%d, R%d\n", rd, ra, rb);
      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "AND") == 0){
      rrr.opcode = 0b0010;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "OR") == 0) {
      rrr.opcode = 0b0011;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "XOR") == 0){
      rrr.opcode = 0b0100;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "DIVMOD") == 0){
      rrr.opcode = 0b0101;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "MUL") == 0){ 
      rrr.opcode = 0b0110;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "CMP") == 0){
      rrr.opcode = 0b0111;
      rrr.regD = rd;
      rrr.regA = ra;
      rrr.regB = rb;

      return RRRTypeEncode(&rrr);
    } else if (strcmp(opcode, "LSH") == 0){ // RR Types
      rr.opcode = 0b1000;                     // same opcode for all shifts
      rr.type   = type;                       // 0=LSL,1=LSR,2=ROL,3=ROR
      rr.regA   = ra;                         // Rd
      rr.regB   = rb;                         // Rs (amount)
      
      return RRTypeEncode(&rr);
    } else if (strcmp(opcode, "BEQ") == 0){
      rri.opcode = 0b1011;
      rri.regD = rd;
      rri.regA = ra;
      rri.imm = rb & 0xF; // Limit to 4 bits but allow parsing larger values

      return RRITypeEncode(&rri);
    }
  }
  
  printf("Invalid instruction, [ %s ].\n", line);
  return 0;
}
