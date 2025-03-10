#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "utilities.h"


// Queue Implementation
void initQueue(Queue* q){
  q->front = 0;
  q->rear = 0;
}

bool isEmpty(Queue* q){
  return (q->front == q->rear);
}

bool isFull(Queue* q){
  return (q->rear == MAX_SIZE);
}

void enqueue(Queue* q, cmdElement* cmd){
  if (isFull(q)){
    printf("Queue is Full.\n");
    return;
  }

  q->items[q->rear++] = *cmd;
}

cmdElement dequeue(Queue* q){
  cmdElement nullElement = {"", 0, 0};

  if (isEmpty(q)){
    printf("Queue is Empty.\n");
    return nullElement;
  }

  return q->items[q->front++];
}

void displayQueue(Queue* q){
    printf("========================= Queued Commands =========================\n");
    if (isEmpty(q)){
        printf("Queue is Empty\n");
        return;
    }

    for (int i = q->front; i < q->rear; i++) {
        printf("Element %d: {command: \"%s\", execute: %d, address: %d, value: %d}\n",
               i, q->items[i].cmd, q->items[i].execute, q->items[i].addr, q->items[i].value);
    }
}


// Function to check the queue for any command that may be ready to execute.
bool checkQueue(Queue* q, int16_t* cycle){
  cmdElement currentCmd = q->items[q->front];

  if (currentCmd.execute == *cycle){
    return true;
  }

  return false;
}

// Functions to keep track of returns, and current cycle.


void initReturnBuffer(ReturnBuffer* rb) {
  rb->count = 0;
  for (int i = 0; i < MAX_COMMAND_RETURNS; i++) {
    rb->returns[i][0] = '\0';
  }
}

void addCommandReturn(ReturnBuffer* rb, const char* message) {
  if (rb->count < MAX_COMMAND_RETURNS) {
    strncpy(rb->returns[rb->count], message, MAX_RETURN_LENGTH - 1);
    rb->returns[rb->count][MAX_RETURN_LENGTH - 1] = '\0';
    rb->count++;
  } else {
    // If the buffer is full, remove the oldest message and append the new one.
    for (int i = 1; i < MAX_COMMAND_RETURNS; i++) {
      strcpy(rb->returns[i-1], rb->returns[i]);
    }
    strncpy(rb->returns[MAX_COMMAND_RETURNS - 1], message, MAX_RETURN_LENGTH - 1);
    rb->returns[MAX_COMMAND_RETURNS - 1][MAX_RETURN_LENGTH - 1] = '\0';
  }
}

void displayCommandReturns(ReturnBuffer* rb) {
  printf("\n\n===================== Command Returns =====================\n");
  if (rb->count == 0) {
    printf("No command returns yet.\n");
  } else {
    for (int i = 0; i < rb->count; i++) {
      printf("%s\n", rb->returns[i]);
    }
  }
}
