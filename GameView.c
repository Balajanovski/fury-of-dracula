////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// GameView.c: GameView ADT implementation
//
// 2014-07-01	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2017-12-01	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v2.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10   v3.0    Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-19   v4.0    James Balajan
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
#include "LocationDynamicArray.h"

struct gameView {
    bool player_death_states[NUM_PLAYERS];
	int player_healths[NUM_PLAYERS];
	Map map;
	int move_number;
	int score;

	DraculaTrail dracula_trail;
	LocationDynamicArray player_location_histories[NUM_PLAYERS];
	LocationDynamicArray player_move_histories[NUM_PLAYERS];

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

    gv->move_number = 0;
    gv->score = GAME_START_SCORE;

    // Initialise the player death state array
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        gv->player_death_states[i] = false;
    }

    // Encounter initialisation
    gv->vampire_location = NOWHERE;
    for (int i = 0; i < TRAIL_SIZE; ++i) {
        gv->trap_locations[i] = NOWHERE;
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
        case 'D':
            return PLAYER_DRACULA;
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

static int num_traps(GameView gv) {
    int num = 0;
    for (int i = 0; i < TRAIL_SIZE; ++i) {
        if (gv->trap_locations[i] != NOWHERE) {
            ++num;
        }
    }

    return num;
}

static PlaceId* get_traps(GameView gv) {
    PlaceId* trap_locations = (PlaceId*) malloc(sizeof(PlaceId) * num_traps(gv));
    int trap_insert_pos = 0;
    for (int i = 0; i < TRAIL_SIZE; ++i) {
        if (gv->trap_locations[i] != NOWHERE) {
            trap_locations[trap_insert_pos++] = gv->trap_locations[i];
        }
    }

    return trap_locations;
}

static PlaceId apply_hunter_encounters(GameView gv, Player curr_player, PlaceId location, const char* encounters_str) {
    if (gv->player_death_states[curr_player]) {
        gv->player_death_states[curr_player] = false;
        gv->player_healths[curr_player] = GAME_START_HUNTER_LIFE_POINTS;
    }

    if (get_size_location_dynamic_array(gv->player_location_histories[curr_player]) > 0 &&
    location == ith_latest_location_location_dynamic_array(gv->player_location_histories[curr_player], 0)) {
        gv->player_healths[curr_player] += LIFE_GAIN_REST;
        gv->player_healths[curr_player] = MIN(GvGetHealth(gv, curr_player), GAME_START_HUNTER_LIFE_POINTS);
    }

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
        gv->player_healths[curr_player] = MAX(gv->player_healths[curr_player], 0);
        gv->score -= SCORE_LOSS_HUNTER_HOSPITAL;
        gv->player_death_states[curr_player] = true;

        return ST_JOSEPH_AND_ST_MARY;
    }

    return location;
}


static PlaceId apply_dracula_encounters_and_actions(GameView gv, PlaceId location, const char* encounters_and_actions_str) {
    gv->score -= SCORE_LOSS_DRACULA_TURN;

    DraculaMove dracula_move;
    dracula_move.location = location;

    for (int i = 0; i < CHARACTERS_IN_DRACULA_ENCOUNTER; ++i) {
        switch(encounters_and_actions_str[i]) {
            case 'T':
            {
                dracula_move.placed_trap = true;
                add_trap(gv, location);
            }
                break;
            case 'V':
            {
                dracula_move.placed_vampire = true;
                gv->vampire_location = location;
            }
                break;
        }
    }

    // Handle dracula's trail expiry actions
    DraculaMove popped_move;
    bool move_popped = push_trail(gv->dracula_trail, dracula_move, &popped_move);

    bool trap_removed = false;
    bool vampire_matured = false;
    for (int i = 0; i < CHARACTERS_IN_DRACULA_ACTION; ++i) {
        switch(encounters_and_actions_str[i]) {
            case 'M':
            {
                trap_removed = true;
            }
                break;
            case 'V':
            {
                vampire_matured = true;

                gv->score -= SCORE_LOSS_VAMPIRE_MATURES;
                gv->vampire_location = NOWHERE;
            }
                break;
        }
    }
    trap_removed = trap_removed || (move_popped && popped_move.placed_trap);
    vampire_matured = vampire_matured || (move_popped && popped_move.placed_vampire);

    if (trap_removed) {
        remove_trap(gv, location);
    } if (vampire_matured) {
        gv->vampire_location = NOWHERE;
        gv->score -= SCORE_LOSS_VAMPIRE_MATURES;
    }

    // Handle Dracula's health
    if (location == CASTLE_DRACULA) {
        gv->player_healths[PLAYER_DRACULA] += LIFE_GAIN_CASTLE_DRACULA;
    } if (placeIsSea(location)) {
        gv->player_healths[PLAYER_DRACULA] -= LIFE_LOSS_SEA;
    } if (gv->player_healths[PLAYER_DRACULA] < 0) {
        gv->player_healths[PLAYER_DRACULA] = MAX(gv->player_healths[PLAYER_DRACULA], 0);
        gv->player_death_states[PLAYER_DRACULA] = true;
    }

    return location;
}

static PlaceId calculate_absolute_player_location(GameView gv, Player player, PlaceId location) {
    if (placeIsReal(location) ||
        location == CITY_UNKNOWN ||
        location == SEA_UNKNOWN ||
        location == NOWHERE ||
        location == UNKNOWN_PLACE) {
        return location;
    }

    if (location == TELEPORT) {
        return CASTLE_DRACULA;
    } else if (location == HIDE || location == DOUBLE_BACK_1) {
        return calculate_absolute_player_location(
                gv,
                player,
                ith_latest_location_location_dynamic_array(gv->player_location_histories[player], 0)
        );
    } else if (location == DOUBLE_BACK_2) {
        return calculate_absolute_player_location(
                gv,
                player,
                ith_latest_location_location_dynamic_array(gv->player_location_histories[player], 1)
        );
    } else if (location == DOUBLE_BACK_3) {
        return calculate_absolute_player_location(
                gv,
                player,
                ith_latest_location_location_dynamic_array(gv->player_location_histories[player], 2)
        );
    } else if (location == DOUBLE_BACK_4) {
        return calculate_absolute_player_location(
                gv,
                player,
                ith_latest_location_location_dynamic_array(gv->player_location_histories[player], 3)
        );
    } else if (location == DOUBLE_BACK_5) {
        return calculate_absolute_player_location(
                gv,
                player,
                ith_latest_location_location_dynamic_array(gv->player_location_histories[player], 4)
        );
    } else {
        fprintf(stderr, "Unhandled location case %s. Aborting...\n", placeIdToAbbrev(location));
        exit(1);
    }
}

#define PLACE_ABBREV_CHARACTER_LEN 2

// Simulate past plays
static void simulate_past_plays(GameView gv, char* past_plays) {
    // Copy past plays to malloced array in case it is a string literal
    unsigned long past_plays_len = strlen(past_plays);
    char* past_plays_copy = (char*) malloc(past_plays_len * sizeof(char));
    strcpy(past_plays_copy, past_plays);

    const char* delimiters = " \n";
    char* move = strtok(past_plays_copy, delimiters);

    while (move != NULL) {
        Player move_player = player_id_from_move_string(move);

        char* move_abbrev = (char*) malloc(sizeof(char) * (PLACE_ABBREV_CHARACTER_LEN + 1));
        strncpy(move_abbrev, move + 1, PLACE_ABBREV_CHARACTER_LEN);
        move_abbrev[PLACE_ABBREV_CHARACTER_LEN] = '\0';
        PlaceId movement = placeAbbrevToId(move_abbrev);
        free(move_abbrev);

        PlaceId new_loc = calculate_absolute_player_location(gv, move_player, movement);
        if (IS_DRACULA(move_player)) {
            new_loc = apply_dracula_encounters_and_actions(gv, new_loc, move + 1 + PLACE_ABBREV_CHARACTER_LEN);
        } else {
            new_loc = apply_hunter_encounters(gv, move_player, new_loc, move + 1 + PLACE_ABBREV_CHARACTER_LEN);
        }


        push_back_location_dynamic_array(gv->player_move_histories[move_player], movement);
        push_back_location_dynamic_array(gv->player_location_histories[move_player], new_loc);
        ++gv->move_number;

        move = strtok(NULL, delimiters);
    }

    free(past_plays_copy);
}

static PlaceId* get_reachable_by_type(GameView gv, Player player, Round round,
                                      PlaceId from, bool road, bool rail, bool boat, int* num_returned_locs,
                                      bool* visited) {
    if (visited[from]) {
        *num_returned_locs = 0;
        return NULL;
    }
    visited[from] = true;

    ConnList reachable_connections = MapGetConnections(gv->map, from);
    LocationDynamicArray road_reachable_locations = new_location_dynamic_array();
    LocationDynamicArray boat_reachable_locations = new_location_dynamic_array();
    LocationDynamicArray rail_reachable_locations = new_location_dynamic_array();

    struct connNode* iter = reachable_connections;
    while (iter != NULL) {
        unsigned int round_player_sum = player + round;

        if (boat && iter->type == BOAT && (!IS_DRACULA(player) || iter->p != ST_JOSEPH_AND_ST_MARY)) {
            push_back_location_dynamic_array(boat_reachable_locations, iter->p);
        } if (road && iter->type == ROAD && (!IS_DRACULA(player) || iter->p != ST_JOSEPH_AND_ST_MARY)) {
            push_back_location_dynamic_array(road_reachable_locations, iter->p);
        } if (rail && iter->type == RAIL && !IS_DRACULA(player) && (round_player_sum % 4) > 0) {
            int num_returned_further_rail_links;
            PlaceId* further_rail_link_locs = get_reachable_by_type(gv, player, round+1, iter->p, false, true, false, &num_returned_further_rail_links, visited);

            extend_location_dynamic_array_raw(rail_reachable_locations, further_rail_link_locs, num_returned_further_rail_links);
            free(further_rail_link_locs);
        }

        iter = iter->next;
    }

    // Put all of the locations into rail_reachable_locations
    extend_location_dynamic_array(rail_reachable_locations, road_reachable_locations);
    extend_location_dynamic_array(rail_reachable_locations, boat_reachable_locations);
    free_location_dynamic_array(road_reachable_locations);
    free_location_dynamic_array(boat_reachable_locations);

    // The location is of course reachable from itself
    push_back_location_dynamic_array(rail_reachable_locations, from);

    int locations_size;
    PlaceId* raw_locations = copy_to_raw_array_from_index_location_dynamic_array(rail_reachable_locations, 0, &locations_size);
    free_location_dynamic_array(rail_reachable_locations);

    *num_returned_locs = locations_size;
    return raw_locations;
}

/*
static PlaceId obfuscate_move(PlaceId move) {
    if (!placeIsReal(move)) {
        return move;
    }

    if (placeIsLand(move)) {
        return CITY_UNKNOWN;
    } else {
        return SEA_UNKNOWN;
    }
}

static PlaceId* get_obfuscated_copy_of_moves(PlaceId* moves_list, int size) {
    PlaceId* obfuscated_moves_list = (PlaceId*) malloc(sizeof(PlaceId) * size);

    for (int i = 0; i < size; ++i) {
        obfuscated_moves_list[i] = obfuscate_move(moves_list[i]);
    }

    return obfuscated_moves_list;
}
*/

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

	// Create player location histories
	for (int i = 0; i < NUM_PLAYERS; ++i) {
	    new->player_location_histories[i] = new_location_dynamic_array();
	    if (new->player_location_histories[i] == NULL) {
	        fprintf(stderr, "Couldn't allocate location history for player %d\n", i);
	        exit(EXIT_FAILURE);
	    }
	}

	// Create player move histories
	for (int i = 0; i < NUM_PLAYERS; ++i) {
	    new->player_move_histories[i] = new_location_dynamic_array();
	    if (new->player_location_histories[i] == NULL) {
            fprintf(stderr, "Couldn't allocate move history for player %d\n", i);
            exit(EXIT_FAILURE);
	    }
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

    // Free player location histories
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        assert(gv->player_location_histories[i] != NULL);
        free_location_dynamic_array(gv->player_location_histories[i]);
    }

    // Free player move histories
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        assert(gv->player_location_histories[i] != NULL);
        free_location_dynamic_array(gv->player_move_histories[i]);
    }

    free_trail(gv->dracula_trail);
    MapFree(gv->map);
	free(gv);
}

////////////////////////////////////////////////////////////////////////
// Game State Information

Round GvGetRound(GameView gv) {
    assert(gv != NULL);
	return gv->move_number / NUM_PLAYERS;
}

Player GvGetPlayer(GameView gv) {
    assert(gv != NULL);
	return gv->move_number % NUM_PLAYERS;
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
	assert(player >= 0 && (int) player < (int) NUM_PLAYERS);
	assert(gv != NULL);
	return gv->player_healths[player];
}

PlaceId GvGetPlayerLocation(GameView gv, Player player) {
	assert(gv != NULL);
	assert(player >= 0 && (int) player < (int) NUM_PLAYERS);

	int loc_history_size = get_size_location_dynamic_array(gv->player_location_histories[player]);
	if (loc_history_size > 0) {
	    return ith_latest_location_location_dynamic_array(gv->player_location_histories[player], 0);
	} else {
	    return NOWHERE;
	}
}

PlaceId GvGetVampireLocation(GameView gv) {
    assert(gv->vampire_location != SEA_UNKNOWN);
    return gv->vampire_location;
}

PlaceId *GvGetTrapLocations(GameView gv, int *numTraps) {
	*numTraps = num_traps(gv);
	return get_traps(gv);
}

////////////////////////////////////////////////////////////////////////
// Game History

PlaceId *GvGetMoveHistory(GameView gv, Player player,
                          int *numReturnedMoves, bool *canFree) {
    int history_size = get_size_location_dynamic_array(gv->player_move_histories[player]);
    return GvGetLastMoves(gv, player, history_size, numReturnedMoves, canFree);
}

PlaceId *GvGetLastMoves(GameView gv, Player player, int numMoves,
                        int *numReturnedMoves, bool *canFree) {
    LocationDynamicArray desired_history = gv->player_move_histories[player];
    int history_size = get_size_location_dynamic_array(desired_history);

    int returned_moves = history_size - MAX(history_size - numMoves, 0);
    if (returned_moves <= 0) {
        *numReturnedMoves = returned_moves;
        *canFree = false;
        return NULL;
    }

    *canFree = false;
    int raw_size;
    PlaceId* raw_arr = get_raw_array_from_index_location_dynamic_array(desired_history, history_size - returned_moves, &raw_size);
    *numReturnedMoves = raw_size;
    return raw_arr;
}

PlaceId *GvGetLocationHistory(GameView gv, Player player,
                              int *numReturnedLocs, bool *canFree) {
    int history_size = get_size_location_dynamic_array(gv->player_location_histories[player]);
    return GvGetLastLocations(gv, player, history_size, numReturnedLocs, canFree);
}

PlaceId *GvGetLastLocations(GameView gv, Player player, int numLocs,
                            int *numReturnedLocs, bool *canFree) {
    LocationDynamicArray desired_history = gv->player_location_histories[player];
    int history_size = get_size_location_dynamic_array(desired_history);

    int returned_locations = history_size - MAX(history_size - numLocs, 0);
    if (returned_locations <= 0) {
        *numReturnedLocs = returned_locations;
        *canFree = false;
        return NULL;
    }

    int raw_size;
    PlaceId* raw_loc_array = get_raw_array_from_index_location_dynamic_array(desired_history, history_size - returned_locations, &raw_size);
    *numReturnedLocs = raw_size;

    *canFree = false;
    return raw_loc_array;
}

////////////////////////////////////////////////////////////////////////
// Making a Move

PlaceId *GvGetReachable(GameView gv, Player player, Round round,
                        PlaceId from, int *numReturnedLocs) {
    return GvGetReachableByType(gv, player, round, from, true, true, true, numReturnedLocs);
}

PlaceId *GvGetReachableByType(GameView gv, Player player, Round round,
                              PlaceId from, bool road, bool rail,
                              bool boat, int *numReturnedLocs) {
    size_t visited_size = sizeof(bool) * NUM_REAL_PLACES;
    bool* visited = (bool*) malloc(visited_size);
    memset(visited, false, visited_size);

    PlaceId* reachable = get_reachable_by_type(gv, player, round, from, road, rail, boat, numReturnedLocs, visited);

    free(visited);

    return reachable;
}

////////////////////////////////////////////////////////////////////////
// Your own interface functions

// TODO
