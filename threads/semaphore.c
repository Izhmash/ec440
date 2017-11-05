#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "queue.h"

typedef struct {
    int value;
    struct Queue *q; 
	int ready;
} Semaphore;

int sem_init(sem_t *sem, int pshared, unsigned value)
{
    return 0;
}

int sem_wait(sem_t *sem)
{
    return 0;
}

int sem_post(sem_t *sem)
{
    return 0;
}

int sem_destroy(sem_t *sem)
{
    return 0;
}
