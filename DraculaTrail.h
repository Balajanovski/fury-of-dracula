//
// Created by james on 16/7/20.
//

#ifndef FURY_OF_DRACULA_DRACULATRAIL_H
#define FURY_OF_DRACULA_DRACULATRAIL_H

#include <stdbool.h>

#include "DraculaMove.h"


typedef struct draculaTrail* DraculaTrail;

// Constructor
DraculaTrail new_trail();

// Destructor
void free_trail(DraculaTrail trail);

// Push supplied dracula move into the trail
// Pop the oldest move in dracula's trail if the trail exceeds DRACULA_TRAIL_MAX_LENGTH and set popped_move to it
// true is returned if a move was popped. false otherwise
bool push_trail(DraculaTrail trail, DraculaMove move, DraculaMove* popped_move);

// Retrieve the i-th latest move from the trail
DraculaMove get_ith_latest_move_trail(DraculaTrail trail, int i);

// Modifies the i-th lastest move from the trail
void set_ith_latest_move_trail(DraculaTrail trail, int i, DraculaMove move);

// Get size of dracula trail
int get_size_trail(DraculaTrail trail);

#endif //FURY_OF_DRACULA_DRACULATRAIL_H
