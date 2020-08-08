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

static PlaceId* get_valid_moves(DraculaView dv, int* num_returned_moves) {
    assert(dv != NULL);
    assert(num_returned_moves != NULL);

    Round curr_round = DvGetRound(dv);

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
    if (curr_round > 0 && !placeIsSea(DvGetPlayerLocation(dv, PLAYER_DRACULA))) {
        insert_move_set(reachable_locs, HIDE);
    }

    // Filter out any duplicate moves in Dracula's trail
    for (int trail_index = 0; trail_index < MIN(trail_size, TRAIL_SIZE-1); ++trail_index) {
        PlaceId ith_latest_trail_move = get_ith_latest_move_trail(trail, trail_index).move;
        remove_move_set(reachable_locs, ith_latest_trail_move);
    }

    *num_returned_moves = get_size_move_set(reachable_locs);
    PlaceId* filtered_reachable_locs = convert_to_array_move_set(reachable_locs);

    free_move_set(reachable_locs);

    return filtered_reachable_locs;
}

PlaceId *DvGetValidMoves(DraculaView dv, int *numReturnedMoves) {
    assert(dv != NULL);

    PlaceId drac_location = DvGetPlayerLocation(dv, PLAYER_DRACULA);
    if (drac_location == NOWHERE) {
        *numReturnedMoves = 0;
        return NULL;
    }

    return get_valid_moves(dv, numReturnedMoves);
}

PlaceId *DvWhereCanIGo(DraculaView dv, int *numReturnedLocs) {
    assert(dv != NULL);
	return DvWhereCanIGoByType(dv, true, true, numReturnedLocs);
}

PlaceId *DvWhereCanIGoByType(DraculaView dv, bool road, bool boat,
                             int *numReturnedLocs) {
    assert(dv != NULL);
	return DvWhereCanTheyGoByType(dv, PLAYER_DRACULA, road, false, boat, numReturnedLocs);
}

PlaceId *DvWhereCanTheyGo(DraculaView dv, Player player, int *numReturnedLocs) {
    assert(dv != NULL);
    return DvWhereCanTheyGoByType(dv, player, true, player != PLAYER_DRACULA, true, numReturnedLocs);
}

PlaceId *DvWhereCanTheyGoByTypeFromLocationAndRound(DraculaView dv, Player player,
                                                    Round round, PlaceId player_loc,
                                                    bool road, bool rail, bool boat,
                                                    int *numReturnedLocs) {
    assert(dv != NULL);

    if (player_loc == NOWHERE) {
        *numReturnedLocs = 0;
        return NULL;
    }

    // No special cases for dracula's trail need to be handled if the player is a hunter
    if (player != PLAYER_DRACULA) {
        return GvGetReachableByType(dv->gv, player, round, DvGetPlayerLocation(dv, player), road, rail, boat, numReturnedLocs);
    }

    MoveSet reachable_locs = make_adjacent_locations_move_set(dv, player, DvGetPlayerLocation(dv, player), round, road, false, boat);

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

PlaceId *DvWhereCanTheyGoByType(DraculaView dv, Player player,
                                bool road, bool rail, bool boat,
                                int *numReturnedLocs) {
    assert(dv != NULL);

    PlaceId player_loc = DvGetPlayerLocation(dv, player);
    int adjusted_round = DvGetRound(dv) + (player < PLAYER_DRACULA);
    return DvWhereCanTheyGoByTypeFromLocationAndRound(dv, player, adjusted_round, player_loc, road, rail, boat, numReturnedLocs);
}

DraculaView DvMakeCopy(DraculaView dv) {
    assert(dv != NULL);

    DraculaView new = malloc(sizeof(*new));
    if (new == NULL) {
        fprintf(stderr, "Couldn't allocate DraculaView for copy\n");
        exit(EXIT_FAILURE);
    }

    new->gv = GvMakeCopy(dv->gv);

    return new;
}

void DvAdvanceStateByMoves(DraculaView dv, char* play_string) {
    assert(dv != NULL);

    GvAdvanceStateByMoves(dv->gv, play_string);
}

static bool can_place_encounter(PlaceId place, PlaceId vamp_loc, const PlaceId* trap_locs, int num_traps) {
    if (!placeIsLand(place)) {
        return false;
    }

    int num_encounters = (place == vamp_loc);
    for (int i = 0; i < num_traps; ++i) {
        num_encounters += (trap_locs[i] == place);
    }

    return num_encounters < MAX_NUM_ENCOUNTERS_IN_CITY;
}

static char player_to_code(Player player) {
    const static char player_codes[] = {'G', 'S', 'H', 'M', 'D'};
    return player_codes[player];
}

static char** possible_moves_for_dracula(DraculaView dv, int* num_moves_returned) {
    int num_valid_moves = -1;
    PlaceId* possible_moves = get_valid_moves(dv, &num_valid_moves);
    assert(num_valid_moves != -1);

    if (num_valid_moves == 0) {
        // Can only teleport

        free(possible_moves);
        possible_moves = malloc(sizeof(PlaceId));
        if (possible_moves == NULL) {
            fprintf(stderr, "Unable to allocate new possible moves for special dracula teleport case in AI. Aborting...\n");
            exit(EXIT_FAILURE);
        }

        possible_moves[0] = TELEPORT;
        num_valid_moves = 1;
    }

    int num_traps = -1;
    PlaceId* trap_locs = GvGetTrapLocations(dv->gv, &num_traps);
    assert(num_traps != -1);
    PlaceId vampire_loc = GvGetVampireLocation(dv->gv);
    Round current_round = DvGetRound(dv);

    char** move_buffer = malloc(sizeof(char*) * num_valid_moves);
    for (int move_index = 0; move_index < num_valid_moves; ++move_index) {
        char* possible_move_str = malloc(sizeof(char) * (MOVE_STRING_LENGTH + 1));

        // Add move string place and player
        const char* place_abbrev = placeIdToAbbrev(possible_moves[move_index]);
        strcpy(possible_move_str+1, place_abbrev);
        possible_move_str[0] = player_to_code(PLAYER_DRACULA);

        // Set rest of move string to blank
        for (int j = MOVE_STRING_LENGTH - ENCOUNTERS_STRING_LEN; j < MOVE_STRING_LENGTH; ++j) {
            possible_move_str[j] = '.';
        }

        // Add dracula's encounters
        if (can_place_encounter(possible_moves[move_index], vampire_loc, trap_locs, num_traps)) {
            if (current_round % 13 == 0) {
                possible_move_str[MOVE_STRING_LENGTH - ENCOUNTERS_STRING_LEN] = 'V';
            } else {
                possible_move_str[MOVE_STRING_LENGTH - ENCOUNTERS_STRING_LEN] = 'T';
            }
        }

        // Do not have to worry about adding encounter expiry action to move
        // This is automatically handled inside the GameView

        possible_move_str[MOVE_STRING_LENGTH] = '\0';
        move_buffer[move_index] = possible_move_str;
    }

    free(possible_moves);
    free(trap_locs);

    *num_moves_returned = num_valid_moves;
    return move_buffer;
}

static char** possible_moves_for_hunter(DraculaView dv, Player player, int* num_moves_returned) {
    assert(player != PLAYER_DRACULA);

    Round round = DvGetRound(dv);
    int num_valid_moves = -1;
    PlaceId* possible_moves = NULL;
    if (round == 0) {
        possible_moves = (PlaceId*) malloc(sizeof(PlaceId) * NUM_REAL_PLACES);
        for (PlaceId p = 0; p < NUM_REAL_PLACES; ++p) {
            possible_moves[p] = p;
        }
        num_valid_moves = NUM_REAL_PLACES;
    } else {
        possible_moves = DvWhereCanTheyGo(dv, player, &num_valid_moves);
    }
    assert(num_valid_moves != -1);

    int num_traps = -1;
    PlaceId* trap_locs = GvGetTrapLocations(dv->gv, &num_traps);
    assert(num_traps != -1);
    PlaceId vampire_loc = GvGetVampireLocation(dv->gv);

    PlaceId dracula_loc = GvGetPlayerLocation(dv->gv, PLAYER_DRACULA);
    char** move_buffer = malloc(sizeof(char*) * num_valid_moves);

    for (int move_index = 0; move_index < num_valid_moves; ++move_index) {
        char* possible_move_str = malloc(sizeof(char) * (MOVE_STRING_LENGTH + 1));

        // Add move string place and player
        const char* place_abbrev = placeIdToAbbrev(possible_moves[move_index]);
        strcpy(possible_move_str+1, place_abbrev);
        possible_move_str[0] = player_to_code(player);

        // Set rest of move string to blank
        for (int j = MOVE_STRING_LENGTH - ENCOUNTERS_STRING_LEN; j < MOVE_STRING_LENGTH; ++j) {
            possible_move_str[j] = '.';
        }

        // Apply encounters
        int encounter_string_index = MOVE_STRING_LENGTH - ENCOUNTERS_STRING_LEN;
        for (int trap_index = 0; trap_index < num_traps; ++trap_index) {
            if (trap_locs[trap_index] == possible_moves[move_index]) {
                possible_move_str[encounter_string_index++] = 'T';
            }
        }
        if (possible_moves[move_index] == vampire_loc) {
            possible_move_str[encounter_string_index++] = 'V';
        } if (possible_moves[move_index] == dracula_loc) {
            possible_move_str[encounter_string_index] = 'D';
        }

        possible_move_str[MOVE_STRING_LENGTH] = '\0';
        move_buffer[move_index] = possible_move_str;
    }

    free(possible_moves);
    free(trap_locs);

    *num_moves_returned = num_valid_moves;
    return move_buffer;
}

char** DvComputePossibleMovesForPlayer(DraculaView dv, int* num_moves_returned) {
    Player player = GvGetPlayer(dv->gv);

    char** possible_moves;
    if (player == PLAYER_DRACULA) {
        possible_moves = possible_moves_for_dracula(dv, num_moves_returned);
    } else {
        possible_moves = possible_moves_for_hunter(dv, player, num_moves_returned);
    }

    return possible_moves;
}

inline GameCompletionState DvGameState(DraculaView dv) {
    return GvGameState(dv->gv);
}

inline Player DvGetPlayer(DraculaView dv) {
    return GvGetPlayer(dv->gv);
}

const PlaceId* DvGetChronologicalLocationHistory(DraculaView dv, int* num_moves) {
    return GvGetChronologicalLocationHistory(dv->gv, num_moves);
}

////////////////////////////////////////////////////////////////////////
