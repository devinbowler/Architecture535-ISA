#include <stdbool.h>
#include <stdio.h>
#include "utilities.h"


// Queue Implementation
void initQueue(Queue* q){
  q->front = -1;
  q->rear = 0;
}

bool isEmpty(Queue* q){
  return (q->front == q->rear - 1);
}

bool isFull(Queue* q){
  return (q->rear == MAX_SIZE);
}

void enqueue(Queue* q, cmdElement *cmd){
  if (isFull(q)){
    printf("Queue is Full.\n");
    return;
  }
  q->items[q->rear] = cmd;
  q->rear++;
}

cmdElement dequeue(Queue* q){
  cmdElement nullElement = {"", 0, 0};
  if (isEmpty(q)){
    printf("Queue is Empty.\n");
    return nullElement;
  }
  cmdElement command = q->items[q->front];
  q->front = q->front + 1;
  return command;
}

void displayQueue(Queue* q){
  if (isEmpty(q)){
    printf("Queue is Empty"\n);
    return;
  }
  int16_t end = q->rear;
  for (int i = 0; i <= end; i++) {
    printf("Element %d: {command: \"%s\", address: %d}\n", i, q->items[i].str, q->items[i].addr);
  }
}


