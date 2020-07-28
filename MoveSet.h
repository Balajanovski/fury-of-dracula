//
// Created by james on 28/7/20.
//

#ifndef FURY_OF_DRACULA_MOVESET_H
#define FURY_OF_DRACULA_MOVESET_H

#include "Places.h"
#include <stdbool.h>

#define NUMBER_DRACULA_SPECIAL_MOVES 7

typedef struct moveSet* MoveSet;

MoveSet new_move_set();
void free_move_set(MoveSet set);

void insert_move_set(MoveSet set, PlaceId move);
void remove_move_set(MoveSet set, PlaceId move);
bool is_move_in_set(MoveSet set, PlaceId move);

PlaceId* convert_to_array_move_set(MoveSet set);

int get_size_move_set(MoveSet set);

#endif //FURY_OF_DRACULA_MOVESET_H
