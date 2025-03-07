#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SIZE 100
#define CMD_SIZE 3

typedef struct {
  uint16_t items[MAX_SIZE];
  uint16_t front;
  uint16_t rear;
} Queue;

typedef struct {
  char str[CMD_SIZE];
  uint16_t delay;
  uint16_t addr;
  int16_t value;
} cmdElement;

#endif
