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
#include "MoveSet.h"

struct gameView {
    bool player_death_states[NUM_PLAYERS];
	int player_healths[NUM_PLAYERS];
	Map map;
	unsigned int move_number;
	int score;
	bool is_copy;

	DraculaTrail dracula_trail;
	LocationDynamicArray player_location_histories[NUM_PLAYERS];
	LocationDynamicArray player_move_histories[NUM_PLAYERS];
	LocationDynamicArray chronological_player_location_history;

    PlaceId vampire_location;
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
    int actual_trail_size = get_size_trail(gv->dracula_trail);
    for (int i = 0; i < MIN(TRAIL_SIZE, actual_trail_size); ++i) {
        DraculaMove ith_latest_trail_move = get_ith_latest_move_trail(gv->dracula_trail, i);

        if (ith_latest_trail_move.location == location && ith_latest_trail_move.placed_trap) {
            ith_latest_trail_move.placed_trap = false;
            set_ith_latest_move_trail(gv->dracula_trail, i, ith_latest_trail_move);
            break;
        }
    }
}

static int num_traps(GameView gv) {
    int actual_trail_size = get_size_trail(gv->dracula_trail);
    int num = 0;
    for (int i = 0; i < MIN(TRAIL_SIZE, actual_trail_size); ++i) {
        DraculaMove ith_latest_trail_move = get_ith_latest_move_trail(gv->dracula_trail, i);
        num += (ith_latest_trail_move.placed_trap) ? 1 : 0;
    }

    return num;
}

static PlaceId* get_traps(GameView gv) {
    int actual_trail_size = get_size_trail(gv->dracula_trail);
    PlaceId* trap_locations = (PlaceId*) malloc(sizeof(PlaceId) * num_traps(gv));
    int trap_insert_pos = 0;
    for (int i = 0; i < MIN(TRAIL_SIZE, actual_trail_size); ++i) {
        DraculaMove ith_latest_trail_move = get_ith_latest_move_trail(gv->dracula_trail, i);
        if (ith_latest_trail_move.placed_trap) {
            trap_locations[trap_insert_pos++] = ith_latest_trail_move.location;
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
                int trail_size = get_size_trail(gv->dracula_trail);
                for (int j = 0; j < trail_size; ++j) {
                    DraculaMove ith_latest_trail_move = get_ith_latest_move_trail(gv->dracula_trail, j);

                    if (ith_latest_trail_move.placed_vampire) {
                        ith_latest_trail_move.placed_vampire = false;

                        set_ith_latest_move_trail(gv->dracula_trail, j, ith_latest_trail_move);
                        break;
                    }
                }           

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
        gv->score = MAX(gv->score, 0);
        gv->player_death_states[curr_player] = true;

        return ST_JOSEPH_AND_ST_MARY;
    }

    return location;
}


static PlaceId apply_dracula_encounters_and_actions(GameView gv, PlaceId move, PlaceId location, const char* encounters_and_actions_str) {
    gv->score -= SCORE_LOSS_DRACULA_TURN;
    gv->score = MAX(gv->score, 0);

    DraculaMove dracula_move;
    dracula_move.location = location;
    dracula_move.move = move;
    dracula_move.placed_trap = false;
    dracula_move.placed_vampire = false;

    for (int i = 0; i < CHARACTERS_IN_DRACULA_ENCOUNTER; ++i) {
        switch(encounters_and_actions_str[i]) {
            case 'T':
            {
                dracula_move.placed_trap = true;
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
        gv->score = MAX(gv->score, 0);
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

static PlaceId calculate_absolute_player_location(GameView gv, Player player, PlaceId location, int current_index) {
    if (placeIsReal(location) ||
        location == CITY_UNKNOWN ||
        location == SEA_UNKNOWN ||
        location == NOWHERE ||
        location == UNKNOWN_PLACE) {
        return location;
    }

    if (current_index >= get_size_location_dynamic_array(gv->player_location_histories[player])) {
        fprintf(stderr, "Out of bounds recursion in calculate_absolute_player_locations. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    switch (location) {
        case TELEPORT:
            return CASTLE_DRACULA;
        case HIDE: case DOUBLE_BACK_1:
            return calculate_absolute_player_location(
                    gv,
                    player,
                    ith_latest_location_location_dynamic_array(gv->player_location_histories[player], 0 + current_index),
                    0 + current_index
            );
        case DOUBLE_BACK_2:
            return calculate_absolute_player_location(
                    gv,
                    player,
                    ith_latest_location_location_dynamic_array(gv->player_location_histories[player], 1 + current_index),
                    1 + current_index
            );
        case DOUBLE_BACK_3:
            return calculate_absolute_player_location(
                    gv,
                    player,
                    ith_latest_location_location_dynamic_array(gv->player_location_histories[player], 2 + current_index),
                    2 + current_index
            );
        case DOUBLE_BACK_4:
            return calculate_absolute_player_location(
                    gv,
                    player,
                    ith_latest_location_location_dynamic_array(gv->player_location_histories[player], 3 + current_index),
                    3 + current_index
            );
        case DOUBLE_BACK_5:
            return calculate_absolute_player_location(
                    gv,
                    player,
                    ith_latest_location_location_dynamic_array(gv->player_location_histories[player], 4 + current_index),
                    4 + current_index
            );
        default:
            fprintf(stderr, "Unhandled location case %s. Aborting...\n", placeIdToAbbrev(location));
            exit(1);
    }
}

#define PLACE_ABBREV_CHARACTER_LEN 2

// Simulate past plays
static void simulate_past_plays(GameView gv, char* past_plays) {
    // Copy past plays to malloced array in case it is a string literal
    unsigned long past_plays_len = strlen(past_plays);
    char* past_plays_copy = (char*) malloc((past_plays_len + 1) * sizeof(char));
    strcpy(past_plays_copy, past_plays);

    const char* delimiters = " \n";
    char* strtok_save_ptr;
    char* move = strtok_r(past_plays_copy, delimiters, &strtok_save_ptr);

    while (move != NULL) {
        Player move_player = player_id_from_move_string(move);

        char* move_abbrev = (char*) malloc(sizeof(char) * (PLACE_ABBREV_CHARACTER_LEN + 1));
        strncpy(move_abbrev, move + 1, PLACE_ABBREV_CHARACTER_LEN);
        move_abbrev[PLACE_ABBREV_CHARACTER_LEN] = '\0';
        PlaceId movement = placeAbbrevToId(move_abbrev);
        free(move_abbrev);

        if (movement == NOWHERE) {
            fprintf(stderr, "Invalid move to NOWHERE made for player %d in round %d (Maybe there is a typo in the play string?). Aborting...\n", move_player, GvGetRound(gv));
            exit(EXIT_FAILURE);
        }

        PlaceId new_loc = calculate_absolute_player_location(gv, move_player, movement, 0);

        if (IS_DRACULA(move_player)) {
            new_loc = apply_dracula_encounters_and_actions(gv, movement, new_loc, move + 1 + PLACE_ABBREV_CHARACTER_LEN);
        } else {
            new_loc = apply_hunter_encounters(gv, move_player, new_loc, move + 1 + PLACE_ABBREV_CHARACTER_LEN);
        }

        push_back_location_dynamic_array(gv->player_move_histories[move_player], movement);
        push_back_location_dynamic_array(gv->player_location_histories[move_player], new_loc);
        push_back_location_dynamic_array(gv->chronological_player_location_history, new_loc);
        ++gv->move_number;

        move = strtok_r(NULL, delimiters, &strtok_save_ptr);
    }

    free(past_plays_copy);
}

static void get_reachable_by_type(GameView gv, Player player, Round round,
                                      PlaceId from, bool road, bool rail, bool boat,
                                      MoveSet reachable) {
    if (from == NOWHERE) {
        for (PlaceId place = 0; place < NUM_REAL_PLACES; ++place) {
            insert_move_set(reachable, place);
        }
        if (IS_DRACULA(player)) {
            remove_move_set(reachable, ST_JOSEPH_AND_ST_MARY);
        }

        return;
    }

    if (is_move_in_set(reachable, from) || (from == ST_JOSEPH_AND_ST_MARY && IS_DRACULA(player))) {
        return;
    }
    insert_move_set(reachable, from);

    ConnList reachable_connections = MapGetConnections(gv->map, from);

    unsigned int round_player_sum = player + round;

    struct connNode* iter = reachable_connections;
    while (iter != NULL) {
        if (rail && iter->type == RAIL && !IS_DRACULA(player) && (round_player_sum % 4) > 0) {
            get_reachable_by_type(gv, player, round-1, iter->p, false, true, false, reachable);
        } if (road && iter->type == ROAD && (!IS_DRACULA(player) || iter->p != ST_JOSEPH_AND_ST_MARY)) {
            get_reachable_by_type(gv, player, round-1, iter->p, false, false, false, reachable);
        } if (boat && iter->type == BOAT && (!IS_DRACULA(player) || iter->p != ST_JOSEPH_AND_ST_MARY)) {
            get_reachable_by_type(gv, player, round-1, iter->p, false, false, false, reachable);
        }

        iter = iter->next;
    }
}

void GvAdvanceStateByMoves(GameView gv, char* play_string) {
    simulate_past_plays(gv, play_string);
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
    new->is_copy = false;

    new->chronological_player_location_history = new_location_dynamic_array();
    if (new->chronological_player_location_history == NULL) {
        fprintf(stderr, "Couldn't allocate chronological player history\n");
        exit(EXIT_FAILURE);
    }

	set_default_gamestate(new);
	simulate_past_plays(new, past_plays);

	return new;
}

void GvFree(GameView gv) {
    assert(gv != NULL);
    assert(gv->dracula_trail != NULL);

    // Free player location histories
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        assert(gv->player_location_histories[i] != NULL);
        free_location_dynamic_array(gv->player_location_histories[i]);
    }

    // Free player move histories
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        assert(gv->player_move_histories[i] != NULL);
        free_location_dynamic_array(gv->player_move_histories[i]);
    }

    if (gv->dracula_trail != NULL) {
        free_trail(gv->dracula_trail);
    }

    free_location_dynamic_array(gv->chronological_player_location_history);

    if (!gv->is_copy) {
        MapFree(gv->map);
    }

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
    assert(gv != NULL);
    assert(gv->vampire_location != SEA_UNKNOWN);    
    return gv->vampire_location;
}

PlaceId *GvGetTrapLocations(GameView gv, int *numTraps) {
    assert(gv != NULL);
	*numTraps = num_traps(gv);
	return get_traps(gv);
}

////////////////////////////////////////////////////////////////////////
// Game History

PlaceId *GvGetMoveHistory(GameView gv, Player player,
                          int *numReturnedMoves, bool *canFree) {
    assert(gv != NULL);

    int history_size = get_size_location_dynamic_array(gv->player_move_histories[player]);
    return GvGetLastMoves(gv, player, history_size, numReturnedMoves, canFree);
}

PlaceId *GvGetLastMoves(GameView gv, Player player, int numMoves,
                        int *numReturnedMoves, bool *canFree) {
    assert(gv != NULL);

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
    assert(gv != NULL);

    int history_size = get_size_location_dynamic_array(gv->player_location_histories[player]);
    return GvGetLastLocations(gv, player, history_size, numReturnedLocs, canFree);
}

PlaceId *GvGetLastLocations(GameView gv, Player player, int numLocs,
                            int *numReturnedLocs, bool *canFree) {
    assert(gv != NULL);

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
    assert(gv != NULL);

    return GvGetReachableByType(gv, player, round, from, true, true, true, numReturnedLocs);
}

PlaceId *GvGetReachableByType(GameView gv, Player player, Round round,
                              PlaceId from, bool road, bool rail,
                              bool boat, int *numReturnedLocs) {
    assert(gv != NULL);

    MoveSet reachable = new_move_set();
    get_reachable_by_type(gv, player, round, from, road, rail, boat, reachable);

    PlaceId* reachable_array = convert_to_array_move_set(reachable);
    *numReturnedLocs = get_size_move_set(reachable);

    free_move_set(reachable);

    return reachable_array;
}

////////////////////////////////////////////////////////////////////////
// Custom interface functions

PlaceId GvGetLatestRevealedDraculaPosition(GameView gv, Round* round) {
    assert(gv != NULL);

    LocationDynamicArray dracula_loc_history = gv->player_location_histories[PLAYER_DRACULA];
    for (int i = get_size_location_dynamic_array(dracula_loc_history) - 1; i >= 0; --i) {
        PlaceId ith_latest_loc = ith_location_location_dynamic_array(dracula_loc_history, i);
        if (placeIsReal(ith_latest_loc)) {
            *round = i;
            return ith_latest_loc;
        }
    }

    *round = -1;
    return NOWHERE;
}

DraculaTrail GvGetDraculaTrail(GameView gv) {
    assert(gv != NULL);

    return gv->dracula_trail;
}

GameView GvMakeCopy(GameView gv) {
    GameView new_gv = malloc(sizeof(*new_gv));
    if (new_gv == NULL) {
        fprintf(stderr, "Could not copy GameView. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    new_gv->map = gv->map; // Map is not copied, yet rather shared since it is immutable

    for (int i = 0; i < NUM_PLAYERS; ++i) {
        new_gv->player_location_histories[i] = make_copy_location_dynamic_array(gv->player_location_histories[i]);
        new_gv->player_move_histories[i] = make_copy_location_dynamic_array(gv->player_move_histories[i]);
        new_gv->player_death_states[i] = gv->player_death_states[i];
        new_gv->player_healths[i] = gv->player_healths[i];
    }

    new_gv->dracula_trail = copy_trail(gv->dracula_trail);
    new_gv->move_number = gv->move_number;
    new_gv->score = gv->score;
    new_gv->vampire_location = gv->vampire_location;
    new_gv->is_copy = true;
    new_gv->chronological_player_location_history = make_copy_location_dynamic_array(gv->chronological_player_location_history);

    return new_gv;
}

GameCompletionState GvGameState(GameView gv) {
    if (gv->player_healths[PLAYER_DRACULA] <= 0) {
        return HUNTERS_WIN;
    } else if (gv->score <= 0) {
        return DRACULA_WINS;
    } else {
        return GAME_NOT_OVER;
    }
}

const PlaceId* GvGetChronologicalLocationHistory(GameView gv, int* num_moves) {
    return get_raw_array_from_index_location_dynamic_array(gv->chronological_player_location_history, 0, num_moves);
}

bool GvIsCopy(GameView gv) {
    return gv->is_copy;
}
