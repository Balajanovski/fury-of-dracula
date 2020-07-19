//Queue ADT
//Inspired by cs2521 Week 7 Tutorial
//By Finn Button

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "Queue.h"
#include "Places.h"

typedef struct QueueNode {
	PlaceId place;
	Node next;
} QueueNode;

typedef struct QueueRep {
	QueueNode *head; // ptr to first node
	QueueNode *tail; // ptr to last node
	size_t size;
} QueueRep;

// create new Queue
Queue NewQueue (void)
{
	QueueRep *new = malloc(sizeof (*new));
	new->head = NULL;
	new->tail = NULL;
	new->size = 0;
	return new;
}

static Node NewNode(PlaceId place) {
    
    QueueNode *new = malloc(sizeof(*new));
    assert(new != NULL);
    new->next = NULL;
    new->place = place;
    return new;
}

// free memory from Queue
void FreeQueue (Queue queue)
{
	assert (queue != NULL);
	for (QueueNode *current = queue->head, *next; current != NULL; current = next) {
		next = current->next;
		free (current);
	}
	free (queue);
}

// add place to end of Queue
void AddtoQueue (Queue queue, PlaceId place)
{
	assert (queue != NULL);

	QueueNode *new = NewNode(place);	
	if (queue->size == 0) {
	    queue->head = new;
	    queue->tail = queue->head;	
	} else {
	    queue->tail->next = new;
	    queue->tail = new;
	}
	queue->size++;
}

// remove place from start of Queue
PlaceId RemovefromQueue (Queue queue)
{
	assert (queue != NULL);
	assert (queue->size != 0);
	PlaceId place = queue->head->place;
	QueueNode *tmp = queue->head;
	queue->head = tmp->next;
	free(tmp);
	queue->size--;
	return place;
}

// return the number of places in queue
size_t QueueSize (Queue queue)
{
    return (queue->size);
}
