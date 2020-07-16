////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// GameView.c: GameView ADT implementation
//
// 2014-07-01	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2017-12-01	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v2.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10   v3.0    Team Dracula <cs2521@cse.unsw.edu.au>
//
////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "Game.h"
#include "GameView.h"
#include "Map.h"
#include "Places.h"

struct gameView {
	int player_healths[NUM_PLAYERS];
	PlaceId player_locations[NUM_PLAYERS];
	PlaceId vampire_location;
	Map map;
	int round_number;
	Player current_player;
	int score;
};

////////////////////////////////////////////////////////////////////////
// Private methods

#define IS_DRACULA(player_id) ((player_id) == PLAYER_DRACULA)
#define IS_HUNTER(player_id) (!IS_DRACULA(player_id))

// Sets the game view's state to its default beginning state
static inline void set_default_gamestate(GameView gv) {
    // Set player healths
    for (int player = 0; player < NUM_PLAYERS; ++player) {
        if (IS_DRACULA((player))) {
            gv->player_healths[player] = GAME_START_BLOOD_POINTS;
        } else if (IS_HUNTER((player))) {
            gv->player_healths[player] = GAME_START_HUNTER_LIFE_POINTS;
        } else {
            fprintf(stderr, "Anomalous player who is neither a hunter nor dracula detected. Aborting...\n");
            exit(EXIT_FAILURE);
        }
    }

    // Set player locations to dummy value which represents an unchosen location
    for (int player = 0; player < NUM_PLAYERS; ++player) {
        gv->player_locations[player] = NOWHERE;
    }

    // Create the game map
    gv->map = MapNew();
    if (gv->map == NULL) {
        fprintf(stderr, "Couldn't allocate map!\n");
        exit(EXIT_FAILURE);
    }

    gv->round_number = 0;
    gv->current_player = PLAYER_LORD_GODALMING;
    gv->score = GAME_START_SCORE;
    gv->vampire_location = NOWHERE;
}

// Convert the location into its unknown equivalent
static inline PlaceId make_location_unknown(PlaceId location) {
    assert(placeIsReal(location));

    if (location == NOWHERE || location == UNKNOWN_PLACE || (location >= CITY_UNKNOWN && location <= TELEPORT)) {
        return location;
    }

    PlaceType loc_type = placeIdToType(location);
    if (loc_type == SEA) {
        return SEA_UNKNOWN;
    } else if (loc_type == LAND) {
        return CITY_UNKNOWN;
    } else {
        fprintf(stderr, "Unhandled case supplied to make_location_unknown of %d. Aborting...\n", location);
        exit(EXIT_FAILURE);
    }
}

static Player player_id_from_move_string(char* move_string) {
    char player_character = move_string[0];

    switch (player_character) {
        case 'G':
            return PLAYER_LORD_GODALMING;
        case 'S':
            return PLAYER_DR_SEWARD;
        case 'H':
            return PLAYER_VAN_HELSING;
        case 'M':
            return PLAYER_MINA_HARKER;
        default:
            fprintf(stderr, "Unhandled case supplied to player_id_from_move_string of %s. Aborting...\n", move_string);
            exit(EXIT_FAILURE);
    }
}

// Simulate past plays
static void simulate_past_plays(GameView gv, char* past_plays) {
    char* next_move_start_ptr = past_plays;

    while (*next_move_start_ptr != '\0') {
        // TODO: Change gv based on moves

        next_move_start_ptr += NUMBER_OF_CHARACTERS_IN_A_MOVE;
    }
}

////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

GameView GvNew(char *past_plays, Message messages[]) {
	GameView new = malloc(sizeof(*new));
	if (new == NULL) {
		fprintf(stderr, "Couldn't allocate GameView!\n");
		exit(EXIT_FAILURE);
	}

	set_default_gamestate(new);

	// TODO: modify gamestate based on pastPlays

	return new;
}

void GvFree(GameView gv) {
    assert(gv != NULL);
    assert(gv->map != NULL);

    free(gv->map);
	free(gv);
}

////////////////////////////////////////////////////////////////////////
// Game State Information

Round GvGetRound(GameView gv) {
    assert(gv != NULL);
	return gv->round_number;
}

Player GvGetPlayer(GameView gv) {
    assert(gv != NULL);
	return gv->current_player;
}

int GvGetScore(GameView gv) {
    assert(gv != NULL);
	int score = gv->score;

#ifndef NDEBUG
	if (score < 0 || score > 366) {
	    fprintf(stderr, "Score anomalously found to be outside allowed interval [0, 366]. Aborting....\n");
	    exit(EXIT_FAILURE);
	}
#endif

	return score;
}

int GvGetHealth(GameView gv, Player player) {
	assert(player >= 0 && player < NUM_PLAYERS);
	assert(gv != NULL);
	return gv->player_healths[player];
}

PlaceId GvGetPlayerLocation(GameView gv, Player player) {
	assert(gv != NULL);
	assert(player >= 0 && player < NUM_PLAYERS);
	int actual_loc = gv->player_locations[player];

	if (actual_loc == NOWHERE || IS_HUNTER(player)) {
	    return actual_loc;
	} else if (IS_DRACULA(player)) {
	    // TODO: Implement move reveal logic
	    return make_location_unknown(actual_loc);
	} else {
	    fprintf(stderr, "Unknown case in GvGetPlayerLocation. Aborting....\n");
	    exit(EXIT_FAILURE);
	}
}

PlaceId GvGetVampireLocation(GameView gv) {
    // TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
    return NOWHERE;
}

PlaceId *GvGetTrapLocations(GameView gv, int *numTraps)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*numTraps = 0;
	return NULL;
}

////////////////////////////////////////////////////////////////////////
// Game History

PlaceId *GvGetMoveHistory(GameView gv, Player player,
                          int *numReturnedMoves, bool *canFree)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*numReturnedMoves = 0;
	*canFree = false;
	return NULL;
}

PlaceId *GvGetLastMoves(GameView gv, Player player, int numMoves,
                        int *numReturnedMoves, bool *canFree)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*numReturnedMoves = 0;
	*canFree = false;
	return NULL;
}

PlaceId *GvGetLocationHistory(GameView gv, Player player,
                              int *numReturnedLocs, bool *canFree)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*numReturnedLocs = 0;
	*canFree = false;
	return NULL;
}

PlaceId *GvGetLastLocations(GameView gv, Player player, int numLocs,
                            int *numReturnedLocs, bool *canFree)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*numReturnedLocs = 0;
	*canFree = false;
	return 0;
}

////////////////////////////////////////////////////////////////////////
// Making a Move

PlaceId *GvGetReachable(GameView gv, Player player, Round round,
                        PlaceId from, int *numReturnedLocs)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*numReturnedLocs = 0;
	return NULL;
}

PlaceId *GvGetReachableByType(GameView gv, Player player, Round round,
                              PlaceId from, bool road, bool rail,
                              bool boat, int *numReturnedLocs)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*numReturnedLocs = 0;
	return NULL;
}

////////////////////////////////////////////////////////////////////////
// Your own interface functions

// TODO
