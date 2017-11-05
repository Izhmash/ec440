#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

struct Queue
{
    int front, rear, size;
    unsigned capacity;
    int* array;
};
 
// function to create a queue of given capacity. 
// It initializes size of queue as 0
struct Queue* init_queue(unsigned capacity);
 
// Queue is full when size becomes equal to the capacity 
int isFull(struct Queue* queue);
 
// Queue is empty when size is 0
int isEmpty(struct Queue* queue);
 
// Function to add an item to the queue.  
// It changes rear and size
void enqueue(struct Queue* queue, int item);
 
// Function to remove an item from queue. 
// It changes front and size
int dequeue(struct Queue* queue);

#endif
