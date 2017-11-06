#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdio.h>
#include <stdlib.h>

struct Queue
{
    int front, rear, size;
    unsigned capacity;
    int* array;
};

// Referencing http://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/

struct Queue* init_queue(unsigned capacity)
{
    struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0; 
    queue->rear = capacity - 1;  // This is important, see the enqueue
    queue->array = (int*) malloc(queue->capacity * sizeof(int));
    return queue;
}
 
// Check if queue hits capacity
int is_full(struct Queue* queue)
{  
    return (queue->size == queue->capacity);  
}
 
// Check if queue holds no values
int is_empty(struct Queue* queue)
{  
    return (queue->size == 0); 
}
 
// Add item to queue; edit rear and size
void enqueue(struct Queue* queue, int item)
{
    if (is_full(queue))
        return;
    queue->rear = (queue->rear + 1)%queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
    printf("%d enqueued to queue\n", item);
}
 
// Add item to queue; edit front and size
int dequeue(struct Queue* queue)
{
    // Return negative 1 since pthread_self() > 0
    if (is_empty(queue))
        return -1;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1)%queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

#endif
