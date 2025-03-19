#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include <ctype.h> 
#include "memory.h"

void test(REGISTERS *registers, DRAM *dram, Cache *cache){
  printf("Test for register 0 : %d.\n", registers->R0);
  printf("Test for register 1 : %d.\n", registers->R1);
  printf("Test for register 2 : %d.\n", registers->R2);
  printf("Test for register SR : %d.\n", registers->SR);
}

int main(){
  REGISTERS *registers;
  registers = init_registers();

  // Initialize DRAM.
  DRAM dram;
  clearMemory(&dram);
  dram.state = DRAM_IDLE;
  dram.delayCounter = 0;
  dram.pendingAddr = 0;
  dram.pendingValue = 0;
  strcpy(dram.pendingCmd, "");
  
  // Set up the cache pointer.
  Cache *cache = NULL;
  cache = init_cache(1);
  
  test(registers, &dram, cache);
  return 0;
}

