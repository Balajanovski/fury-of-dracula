//interface for Queue ADT
//By Finn Button

#ifndef QUEUE_H
#define QUEUE_H

#include "Places.h"

typedef struct QueueRep *Queue;

typedef struct QueueNode *Node;

//create a new queue
Queue newQueue (void);

// free memory of queue
void dropQueue (Queue);

// add place to queue
void QueueJoin (Queue, PlaceId);

// delete a place from queue
PlaceId QueueLeave (Queue);

// return number of places in Queue
size_t QueueSize (Queue);

#endif
