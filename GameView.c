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
	int player_healths[NUM_PLAYERS];
	PlaceId player_locations[NUM_PLAYERS];
	Map map;
	int move_number;
	int score;

	DraculaTrail dracula_trail;
	LocationDynamicArray player_location_histories[NUM_PLAYERS];

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

    gv->move_number = 0;
    gv->score = GAME_START_SCORE;

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
        gv->player_locations[curr_player] = ST_JOSEPH_AND_ST_MARY;
        gv->score -= SCORE_LOSS_HUNTER_HOSPITAL;
    }
}


static void apply_dracula_encounters_and_actions(GameView gv, PlaceId location, const char* encounters_and_actions_str) {
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

    DraculaMove popped_move;
    bool move_popped = push_trail(gv->dracula_trail, dracula_move, &popped_move);
    for (int i = 0; i < CHARACTERS_IN_DRACULA_ACTION; ++i) {
        switch(encounters_and_actions_str[i]) {
            case 'M':
            {
#ifndef NDEBUG
                if (!move_popped || !popped_move.placed_trap) {
                    fprintf(stderr, "Dracula trail has drifted from past moves in regards to trap removed from place: %s\n", placeIdToName(location));
                }
#endif

                remove_trap(gv, location);
            }
                break;
            case 'V':
            {
#ifndef NDEBUG
                if (!move_popped || !popped_move.placed_vampire) {
                    fprintf(stderr, "Dracula trail has drifted from past moves in regards to vampire matured from place: %s\n", placeIdToName(location));
                }
#endif

                gv->score -= SCORE_LOSS_VAMPIRE_MATURES;
            }
                break;
        }
    }

    if (location == CASTLE_DRACULA) {
        gv->player_healths[PLAYER_DRACULA] += LIFE_GAIN_CASTLE_DRACULA;
    }
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

// Simulate past plays
static void simulate_past_plays(GameView gv, char* past_plays) {
    const char* delimiters = " \n";
    char* move = strtok(past_plays, delimiters);

    while (move != NULL) {
        Player move_player = player_id_from_move_string(move);
        PlaceId new_loc = calculate_absolute_player_location(gv, move_player, placeAbbrevToId(move+1));
        push_back_location_dynamic_array(gv->player_location_histories[move_player], new_loc);

        if (IS_DRACULA(move_player)) {
            apply_dracula_encounters_and_actions(gv, new_loc, move + 2);
        } else {
            apply_hunter_encounters(gv, move_player, new_loc, move + 2);
        }

        gv->player_locations[move_player] = new_loc;
        ++gv->move_number;

        move = strtok(NULL, delimiters);
    }
}

static PlaceId obfuscate_move(PlaceId move) {
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

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        assert(gv->player_location_histories[i] != NULL);
        free_location_dynamic_array(gv->player_location_histories[i]);
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
	assert(player >= 0 && player < NUM_PLAYERS);
	assert(gv != NULL);
	return gv->player_healths[player];
}

PlaceId GvGetPlayerLocation(GameView gv, Player player) {
	assert(gv != NULL);
	assert(player >= 0 && player < NUM_PLAYERS);
	PlaceId loc = gv->player_locations[player];

	return calculate_absolute_player_location(gv, player, loc);
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
    LocationDynamicArray desired_history = gv->player_location_histories[player];

	*canFree = false;

	int raw_size;
	PlaceId* raw_arr = get_raw_array_from_index_location_dynamic_array(desired_history, 0, &raw_size);
	*numReturnedMoves = raw_size;
	return raw_arr;
}

PlaceId *GvGetLastMoves(GameView gv, Player player, int numMoves,
                        int *numReturnedMoves, bool *canFree) {
    LocationDynamicArray desired_history = gv->player_location_histories[player];
    int history_size = get_size_location_dynamic_array(desired_history);

    int returned_moves = history_size - MAX(history_size - numMoves, 0);

	*numReturnedMoves = returned_moves;
	*canFree = false;

	int raw_size;
	return (returned_moves > 0) ? get_raw_array_from_index_location_dynamic_array(desired_history, history_size - returned_moves, &raw_size) : NULL;
}

PlaceId *GvGetLocationHistory(GameView gv, Player player,
                              int *numReturnedLocs, bool *canFree) {
    bool can_free_move_history;
    PlaceId* move_history = GvGetMoveHistory(gv, player, numReturnedLocs, &can_free_move_history);

    if (IS_DRACULA(player)) {
        PlaceId* location_history = get_obfuscated_copy_of_moves(move_history, *numReturnedLocs);

        if (can_free_move_history) {
            free(move_history);
        }
        *canFree = true;

        return location_history;
    } else {
        *canFree = can_free_move_history;
        return move_history;
    }
}

PlaceId *GvGetLastLocations(GameView gv, Player player, int numLocs,
                            int *numReturnedLocs, bool *canFree) {
    bool can_free_last_move_history;
    PlaceId* last_move_history = GvGetLastMoves(gv, player, numLocs, numReturnedLocs, &can_free_last_move_history);

    if (IS_DRACULA(player)) {
        PlaceId* location_history = get_obfuscated_copy_of_moves(last_move_history, *numReturnedLocs);

        if (can_free_last_move_history) {
            free(last_move_history);
        }
        *canFree = true;

        return location_history;
    } else {
        *canFree = can_free_last_move_history;
        return last_move_history;
    }
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
            PlaceId* further_rail_link_locs = GvGetReachableByType(gv, player, round+1, iter->p, false, true, false, &num_returned_further_rail_links);

            extend_location_dynamic_array_raw(rail_reachable_locations, further_rail_link_locs, num_returned_further_rail_links);
            free(further_rail_link_locs);

            push_back_location_dynamic_array(rail_reachable_locations, iter->p);
        }

        iter = iter->next;
    }

    // Put all of the locations into rail_reachable_locations
    extend_location_dynamic_array(rail_reachable_locations, road_reachable_locations);
    extend_location_dynamic_array(rail_reachable_locations, boat_reachable_locations);
    free_location_dynamic_array(road_reachable_locations);
    free_location_dynamic_array(boat_reachable_locations);

    int locations_size;
    PlaceId* raw_locations = copy_to_raw_array_from_index_location_dynamic_array(rail_reachable_locations, 0, &locations_size);
    free_location_dynamic_array(rail_reachable_locations);

	*numReturnedLocs = locations_size;
	return raw_locations;
}

////////////////////////////////////////////////////////////////////////
// Your own interface functions

// TODO
