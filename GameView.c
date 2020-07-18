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
#include <string.h>

#include "Game.h"
#include "GameView.h"
#include "Map.h"
#include "Places.h"
#include "DraculaTrail.h"

struct gameView {
	int player_healths[NUM_PLAYERS];
	PlaceId player_locations[NUM_PLAYERS];
	Map map;
	int round_number;
	Player current_player;
	int score;
	DraculaTrail dracula_trail;

    PlaceId vampire_location;
	PlaceId trap_locations[TRAIL_SIZE];
};

////////////////////////////////////////////////////////////////////////
// Private methods

#define IS_DRACULA(player_id) ((player_id) == PLAYER_DRACULA)
#define IS_HUNTER(player_id) (!IS_DRACULA(player_id))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

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

    gv->round_number = 0;
    gv->current_player = PLAYER_LORD_GODALMING;
    gv->score = GAME_START_SCORE;
    gv->vampire_location = NOWHERE;

    for (int i = 0; i < TRAIL_SIZE; ++i) {
        gv->trap_locations[i] = NOWHERE;
    }
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

static void remove_trap(GameView gv, PlaceId location) {
    for (int i = 0; i < TRAIL_SIZE; ++i) {
        if (gv->trap_locations[i] == location) {
            gv->trap_locations[i] = NOWHERE;
            break;
        }
    }
}

static void add_trap(GameView gv, PlaceId location) {
    for (int i = 0; i < TRAIL_SIZE; ++i) {
        if (gv->trap_locations[i] == NOWHERE) {
            gv->trap_locations[i] = location;
            break;
        }
    }
}

static void apply_hunter_encounters(GameView gv, Player curr_player, PlaceId location, const char* encounters_str) {
    for (int i = 0; i < ENCOUNTERS_STRING_LEN && gv->player_healths[curr_player] > 0; ++i) {
        switch(encounters_str[i]) {
            case 'T':
            {
                gv->player_healths[curr_player] -= LIFE_LOSS_TRAP_ENCOUNTER;
                remove_trap(gv, location);
            }
                break;
            case 'V':
            {
                gv->vampire_location = NOWHERE;
            }
                break;
            case 'D':
            {
                gv->player_healths[curr_player] -= LIFE_LOSS_DRACULA_ENCOUNTER;
                gv->player_healths[PLAYER_DRACULA] -= LIFE_LOSS_HUNTER_ENCOUNTER;
            }
                break;
        }
    }

    if (gv->player_healths[curr_player] <= 0) {
        gv->player_healths[curr_player] = GAME_START_HUNTER_LIFE_POINTS;
        gv->player_locations[curr_player] = HOSPITAL_PLACE;
        gv->score -= SCORE_LOSS_HUNTER_HOSPITAL;
    }
}


static void apply_dracula_encounters_and_actions(GameView gv, PlaceId location, const char* encounters_and_actions_str) {
    for (int i = 0; i < CHARACTERS_IN_DRACULA_ENCOUNTER; ++i) {
        switch(encounters_and_actions_str[i]) {
            case 'T':
            {
                add_trap(gv, location);
            }
                break;
            case 'V':
            {
                gv->vampire_location = location;
            }
                break;
        }
    }

    for (int i = 0; i < CHARACTERS_IN_DRACULA_ACTION; ++i) {
        switch(encounters_and_actions_str[i]) {
            case 'M':
            {
                remove_trap(gv, location);
            }
                break;
            case 'V':
            {
                gv->score -= SCORE_LOSS_VAMPIRE_MATURES;
            }
                break;
        }
    }
}

// Simulate past plays
static void simulate_past_plays(GameView gv, char* past_plays) {
    const char* delimiters = " \n";
    char* move = strtok(past_plays, delimiters);

    while (move != NULL) {
        Player move_player = player_id_from_move_string(move);
        PlaceId new_loc = placeAbbrevToId(move+1);

        if (IS_DRACULA(move_player)) {
            apply_dracula_encounters_and_actions(gv, new_loc, move + 2);
        } else {
            apply_hunter_encounters(gv, gv->current_player, new_loc, move+2);
        }

        gv->player_locations[move_player] = new_loc;
        ++gv->round_number;
        gv->current_player = (move_player+1) % NUM_PLAYERS;

        move = strtok(NULL, delimiters);
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

	// Create Dracula's trail of destruction
	new->dracula_trail = new_trail();
	if (new->dracula_trail == NULL) {
        fprintf(stderr, "Couldn't allocate dracula trail!\n");
        exit(EXIT_FAILURE);
	}

    // Create the game map
    new->map = MapNew();
    if (new->map == NULL) {
        fprintf(stderr, "Couldn't allocate map!\n");
        exit(EXIT_FAILURE);
    }

	set_default_gamestate(new);

	simulate_past_plays(new, past_plays);

	return new;
}

void GvFree(GameView gv) {
    assert(gv != NULL);
    assert(gv->map != NULL);
    assert(gv->dracula_trail != NULL);

    free(gv->dracula_trail);
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
