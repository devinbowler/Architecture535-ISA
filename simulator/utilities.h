#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_SIZE 100
#define CMD_SIZE 3

typedef struct {
  char cmd[CMD_SIZE];
  uint16_t execute;
  uint16_t addr;
  int16_t value;
} cmdElement;

typedef struct {
  cmdElement items[MAX_SIZE];
  uint16_t front;
  uint16_t rear;
} Queue;

void initQueue(Queue* q);
bool isEmpty(Queue* q);
bool isFull(Queue* q);
void enqueue(Queue* q, cmdElement* cmd);
cmdElement dequeue(Queue* q);
void displayQueue(Queue* q);

#endif
