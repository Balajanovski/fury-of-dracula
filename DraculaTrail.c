//
// Created by james on 16/7/20.
//

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include "Places.h"
#include "DraculaMove.h"
#include "DraculaTrail.h"
#include "Game.h"

struct draculaTrail {
    DraculaMove* queue_array;
    int queue_start_index;
    int queue_end_index;
    int num_elements;
};

/*
 * ADT implementation documentation:
 *
 * Essentially, it is an adhoc implementation of a queue with a fixed size.
 * Whenever an element is pushed into it, it is added at the end index (which then gets incremented and modded by the max trail length)
 * Whenever an element is popped from it, the start index is incremented
 */
DraculaTrail new_trail() {
    DraculaTrail trail = (DraculaTrail) malloc(sizeof(*trail));
    if (trail == NULL) {
        fprintf(stderr, "Unable to malloc dracula trail. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    trail->queue_array = (DraculaMove*) malloc(sizeof(DraculaMove) * TRAIL_SIZE);
    if (trail->queue_array == NULL) {
        fprintf(stderr, "Unable to malloc dracula trail queue. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    trail->queue_start_index = 0;
    trail->queue_end_index = 0;
    trail->num_elements = 0;

    return trail;
}

void free_trail(DraculaTrail trail) {
    assert(trail != NULL);
    assert(trail->queue_array != NULL);

    free(trail->queue_array);
    free(trail);
}

bool push_trail(DraculaTrail trail, DraculaMove move, DraculaMove* popped_move) {
    bool move_popped = false;
    if (trail->num_elements == TRAIL_SIZE) {
        move_popped = true;
        *popped_move = trail->queue_array[trail->queue_start_index];

        --trail->num_elements;
        trail->queue_start_index = (trail->queue_start_index + 1) % TRAIL_SIZE;
    }

    ++trail->num_elements;
    trail->queue_array[trail->queue_end_index] = move;
    trail->queue_end_index = (trail->queue_end_index + 1) % TRAIL_SIZE;

    return move_popped;
}

DraculaMove get_ith_latest_move_trail(DraculaTrail trail, int i) {
    assert(trail != NULL);
    assert(i < trail->num_elements);

    return trail->queue_array[(trail->queue_start_index + (trail->num_elements - i - 1)) % TRAIL_SIZE];
}

void set_ith_latest_move_trail(DraculaTrail trail, int i, DraculaMove move) {
    assert(trail != NULL);
    assert(i < trail->num_elements);
    trail->queue_array[(trail->queue_start_index + (trail->num_elements - i - 1)) % TRAIL_SIZE] = move;
}
