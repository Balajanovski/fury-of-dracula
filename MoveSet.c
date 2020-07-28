//
// Created by james on 28/7/20.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "MoveSet.h"

#define SET_SIZE ((NUM_REAL_PLACES) + (NUMBER_DRACULA_SPECIAL_MOVES))

struct moveSet {
    bool moves_set[SET_SIZE];
    int number_inserted_moves;
};

static int map_move_to_index(PlaceId move) {
    if (placeIsReal(move)) {
        return move;
    } else if (move >= HIDE && move <= TELEPORT) {
        return move - HIDE + NUM_REAL_PLACES;
    } else {
        fprintf(stderr, "Unhandled move case mapped to index in MoveSet.c of %d. Aborting...\n", move);
        exit(EXIT_FAILURE);
    }
}


static PlaceId map_index_to_move(int index) {
    if (placeIsReal(index)) {
        return index;
    } else if (index >= NUM_REAL_PLACES && index < SET_SIZE) {
        return index - NUM_REAL_PLACES + HIDE;
    } else {
        fprintf(stderr, "Unhandled index case mapped to move of %d in MoveSet.c. Aborting...\n", index);
        exit(EXIT_FAILURE);
    }
}


MoveSet new_move_set() {
    MoveSet new_set = (MoveSet) malloc(sizeof(struct moveSet));
    if (new_set == NULL) {
        fprintf(stderr, "Unable to allocate new move set. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    new_set->number_inserted_moves = 0;
    memset(new_set->moves_set, false, sizeof(bool) * (SET_SIZE));

    return new_set;
}

void free_move_set(MoveSet set) {
    assert(set != NULL);
    free(set);
}

void insert_move_set(MoveSet set, PlaceId move) {
    if (!set->moves_set[map_move_to_index(move)]) {
        ++set->number_inserted_moves;
    }

    set->moves_set[map_move_to_index(move)] = true;
}

void remove_move_set(MoveSet set, PlaceId move) {
    if (set->moves_set[map_move_to_index(move)]) {
        --set->number_inserted_moves;
    }

    set->moves_set[map_move_to_index(move)] = false;
}

bool is_move_in_set(MoveSet set, PlaceId move) {
    return set->moves_set[map_move_to_index(move)];
}

PlaceId* convert_to_array_move_set(MoveSet set) {
    if (set->number_inserted_moves <= 0) {
        return NULL;
    }

    PlaceId* array = (PlaceId*) malloc(sizeof(PlaceId) * set->number_inserted_moves);
    int array_insert_index = 0;
    for (int i = 0; i < (SET_SIZE); ++i) {
        if (set->moves_set[i]) {
            array[array_insert_index++] = map_index_to_move(i);
        }
    }

    return array;
}

int get_size_move_set(MoveSet set) {
    return set->number_inserted_moves;
}
