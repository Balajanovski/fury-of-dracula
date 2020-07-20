//interface for Queue ADT
//By Finn Button

#ifndef QUEUE_H
#define QUEUE_H

#include "Places.h"

typedef struct QueueRep *Queue;

typedef struct QueueNode *Node;

//create a new queue
Queue NewQueue (void);

// free memory of queue
void FreeQueue (Queue);

// add place to queue
void AddtoQueue (Queue, PlaceId);

// delete a place from queue
PlaceId RemovefromQueue (Queue);

// return number of places in Queue
size_t QueueSize (Queue);

#endif
