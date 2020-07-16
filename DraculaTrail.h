//
// Created by james on 16/7/20.
//

#ifndef FURY_OF_DRACULA_DRACULATRAIL_H
#define FURY_OF_DRACULA_DRACULATRAIL_H

#include <stdbool.h>

#include "DraculaMove.h"

#define DRACULA_TRAIL_MAX_LENGTH 6


typedef struct draculaTrail* DraculaTrail;

// Constructor
DraculaTrail new_trail();

// Destructor
void free_trail(DraculaTrail trail);

// Push supplied dracula move into the trail
// Pop the oldest move in dracula's trail if the trail exceeds DRACULA_TRAIL_MAX_LENGTH and set popped_move to it
// true is returned if a move was popped. false otherwise
bool push(DraculaTrail trail, DraculaMove move, DraculaMove* popped_move);

// Retrieve the i-th latest move from the trail
DraculaMove get_ith_latest_move(DraculaTrail trail, int i);

#endif //FURY_OF_DRACULA_DRACULATRAIL_H
