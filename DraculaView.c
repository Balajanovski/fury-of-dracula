////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// DraculaView.c: the DraculaView ADT implementation
//
// 2014-07-01	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2017-12-01	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v2.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10	v3.0	Team Dracula <cs2521@cse.unsw.edu.au>
//
////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DraculaView.h"
#include "Game.h"
#include "GameView.h"
#include "Map.h"
#include "LocationDynamicArray.h"
#include "MoveSet.h"


#define IS_DOUBLE_BACK_MOVE(move) ((move) >= DOUBLE_BACK_1 && (move) <= DOUBLE_BACK_5)
#define MIN(a,b) (((a)<(b))?(a):(b))


struct draculaView {
	GameView gv;
};

//////////////////////////////////////////////////////////////////////////
// Private methods

static MoveSet make_adjacent_locations_move_set(DraculaView dv, Player player, PlaceId location, Round round, bool road,
                                                bool rail, bool boat) {
    int num_reachable_locs;
    PlaceId* reachable_locs = GvGetReachableByType(dv->gv, player, round, location, road,
                                                   rail, boat, &num_reachable_locs);

    MoveSet set = new_move_set();
    for (int i = 0; i < num_reachable_locs; ++i) {
        insert_move_set(set, reachable_locs[i]);
    }
    free(reachable_locs);

    return set;
}

static bool double_back_made_in_trail(DraculaView dv) {
    DraculaTrail trail = GvGetDraculaTrail(dv->gv);
    int trail_size = get_size_trail(trail);

    bool double_back_made = false;
    for (int i = 0; i < MIN(trail_size, TRAIL_SIZE-1); ++i) {
        DraculaMove ith_latest_trail_move = get_ith_latest_move_trail(trail, i);
        double_back_made = double_back_made || IS_DOUBLE_BACK_MOVE(ith_latest_trail_move.move);
    }

    return double_back_made;
}

////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

DraculaView DvNew(char *pastPlays, Message messages[]) {
	DraculaView new = malloc(sizeof(*new));
	if (new == NULL) {
		fprintf(stderr, "Couldn't allocate DraculaView\n");
		exit(EXIT_FAILURE);
	}

	new->gv = GvNew(pastPlays, messages);

	return new;
}

void DvFree(DraculaView dv) {
	assert(dv != NULL);
	assert(dv->gv != NULL);
	GvFree(dv->gv);
	free(dv);
}

////////////////////////////////////////////////////////////////////////
// Game State Information

Round DvGetRound(DraculaView dv) {
	assert(dv != NULL);
	return GvGetRound(dv->gv);
}

int DvGetScore(DraculaView dv) {
	assert(dv != NULL);
	return GvGetScore(dv->gv);
}

int DvGetHealth(DraculaView dv, Player player) {
	assert(dv != NULL);
	return GvGetHealth(dv->gv, player);
}

PlaceId DvGetPlayerLocation(DraculaView dv, Player player) {
	assert(dv != NULL);
	return GvGetPlayerLocation(dv->gv, player);
}

PlaceId DvGetVampireLocation(DraculaView dv) {
	assert(dv != NULL);
	return GvGetVampireLocation(dv->gv);
}

PlaceId *DvGetTrapLocations(DraculaView dv, int *numTraps) {
	assert(dv != NULL);
	assert(numTraps != NULL);
	return GvGetTrapLocations(dv->gv, numTraps);
}

////////////////////////////////////////////////////////////////////////
// Making a Move

PlaceId *DvGetValidMoves(DraculaView dv, int *numReturnedMoves) {
    PlaceId drac_location = DvGetPlayerLocation(dv, PLAYER_DRACULA);
    if (drac_location == NOWHERE) {
        *numReturnedMoves = 0;
        return NULL;
    }

    // Get all places dracula can travel to in a set data structure
    MoveSet reachable_locs = make_adjacent_locations_move_set(dv, PLAYER_DRACULA,
                                                              DvGetPlayerLocation(dv, PLAYER_DRACULA),
                                                              DvGetRound(dv), true, false, true);

    // Also add hide and double back moves to the set
    DraculaTrail trail = GvGetDraculaTrail(dv->gv);
    int trail_size = get_size_trail(trail);
    if (!double_back_made_in_trail(dv)) {
        for (int trail_index = 0; trail_index < MIN(trail_size, TRAIL_SIZE-1); ++trail_index) {
            DraculaMove trail_move_corresponding_to_doubleback = get_ith_latest_move_trail(trail, trail_index);

            // Corresponding location in trail is adjacent to Dracula
            if (is_move_in_set(reachable_locs, trail_move_corresponding_to_doubleback.location)) {
                insert_move_set(reachable_locs, trail_index + DOUBLE_BACK_1);
            }
        }
    }
    insert_move_set(reachable_locs, HIDE);

    // Filter out any duplicate moves in Dracula's trail
    for (int trail_index = 0; trail_index < MIN(trail_size, TRAIL_SIZE-1); ++trail_index) {
        remove_move_set(reachable_locs, get_ith_latest_move_trail(trail, trail_index).move);
    }

    *numReturnedMoves = get_size_move_set(reachable_locs);
    PlaceId* filtered_reachable_locs = convert_to_array_move_set(reachable_locs);

    free_move_set(reachable_locs);

    return filtered_reachable_locs;
}

PlaceId *DvWhereCanIGo(DraculaView dv, int *numReturnedLocs) {
	return DvWhereCanIGoByType(dv, true, true, numReturnedLocs);
}

PlaceId *DvWhereCanIGoByType(DraculaView dv, bool road, bool boat,
                             int *numReturnedLocs) {
	return DvWhereCanTheyGoByType(dv, PLAYER_DRACULA, road, false, boat, numReturnedLocs);
}

PlaceId *DvWhereCanTheyGo(DraculaView dv, Player player, int *numReturnedLocs) {
    return DvWhereCanTheyGoByType(dv, player, true, player != PLAYER_DRACULA, true, numReturnedLocs);
}

PlaceId *DvWhereCanTheyGoByType(DraculaView dv, Player player,
                                bool road, bool rail, bool boat,
                                int *numReturnedLocs) {
    if (DvGetPlayerLocation(dv, player) == NOWHERE) {
        *numReturnedLocs = 0;
        return NULL;
    }

    int adjusted_round = DvGetRound(dv) + (player < PLAYER_DRACULA);

    // No special cases for dracula's trail need to be handled if the player is a hunter
    if (player != PLAYER_DRACULA) {
        return GvGetReachableByType(dv->gv, player, adjusted_round, DvGetPlayerLocation(dv, player), road, rail, boat, numReturnedLocs);
    }

    MoveSet reachable_locs = make_adjacent_locations_move_set(dv, player, DvGetPlayerLocation(dv, player), adjusted_round, road, false, boat);

    // Filter out any duplicate moves in Dracula's trail
    DraculaTrail trail = GvGetDraculaTrail(dv->gv);
    int trail_size = get_size_trail(trail);
    for (int trail_index = 0; trail_index < MIN(trail_size, TRAIL_SIZE-1); ++trail_index) {
        remove_move_set(reachable_locs, get_ith_latest_move_trail(trail, trail_index).move);
    }

    *numReturnedLocs = get_size_move_set(reachable_locs);
    PlaceId* filtered_reachable_locs = convert_to_array_move_set(reachable_locs);

    free_move_set(reachable_locs);

    return filtered_reachable_locs;
}

////////////////////////////////////////////////////////////////////////
