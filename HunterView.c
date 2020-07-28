////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// HunterView.c: the HunterView ADT implementation
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
#include "HunterView.h"
#include "Map.h"
#include "Places.h"
#include "Queue.h"

struct hunterView {
    GameView gv;

};

////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

HunterView HvNew(char *pastPlays, Message messages[]) {
	HunterView new = malloc(sizeof(*new));
	if (new == NULL) {
		fprintf(stderr, "Couldn't allocate HunterView!\n");
		exit(EXIT_FAILURE);
	}
	new->gv = GvNew(pastPlays, messages);
	

	return new;
}

void HvFree(HunterView hv) {
    assert(hv != NULL);
    GvFree(hv->gv);
	free(hv);
}

////////////////////////////////////////////////////////////////////////
// Game State Information

Round HvGetRound(HunterView hv) {
	assert (hv != NULL);
	return GvGetRound(hv->gv);
}


Player HvGetPlayer(HunterView hv) {
	assert (hv != NULL);
	return GvGetPlayer(hv->gv);
}

int HvGetScore(HunterView hv) {
	assert (hv != NULL);
	return GvGetScore(hv->gv);
}

int HvGetHealth(HunterView hv, Player player) {
	assert (hv != NULL);
	return GvGetHealth(hv->gv, player);
}

PlaceId HvGetPlayerLocation(HunterView hv, Player player) {
    assert (hv != NULL);
    return GvGetPlayerLocation(hv->gv, player);

}

PlaceId HvGetVampireLocation(HunterView hv) {
	assert (hv != NULL);
	return GvGetVampireLocation(hv->gv);
}

////////////////////////////////////////////////////////////////////////
// Utility Functions

PlaceId HvGetLastKnownDraculaLocation(HunterView hv, Round *round) {
	return GvGetLatestRevealedDraculaPosition(hv->gv, round);
}

#define BFS_UNVISITED_VALUE -1
PlaceId *HvGetShortestPathTo(HunterView hv, Player hunter, PlaceId dest,
                             int *pathLength) {
	Queue path_queue = NewQueue();

	int *visited = malloc(sizeof(int) * NUM_REAL_PLACES);
	for (int i = 0; i < NUM_REAL_PLACES; ++i) {
	    visited[i] = BFS_UNVISITED_VALUE;
	}
	
	// Starting location
	PlaceId source = GvGetPlayerLocation(hv->gv, hunter);
	visited[source] = source;
	AddtoQueue(path_queue, source);

	Round old_round_number = GvGetRound(hv->gv);   // Stored for resetting after BFS
	Round current_round_number = old_round_number; // This fake round number will be updated as we simulate the hunter's movements in the BFS

	bool found = false;
	while (!found && QueueSize(path_queue) > 0) {
	    int queue_size = QueueSize(path_queue);

	    // The queue size at the end of each of these iterations in BFS
	    // is guaranteed to be the size of the next "level" or "layer"
	    // in the BFS (BFS can be visualised as an expanding ring)
	    //
	    // So, at the end of each of these BFS iterations, we know we have gone to the next level
	    // so we can increment the round number (TEMPORARILY)
	    for (int q_level_index = 0; q_level_index < queue_size; ++q_level_index) {
            PlaceId currentLocation = RemovefromQueue(path_queue);

            if (currentLocation == dest) {
                // Early exit once the destination is found
                found = true;
                break;
            } else {
                // Get locations adjacent to the hunter
                int num_adjacent_locations;
                PlaceId *can_go = GvGetReachable(hv->gv, hunter, current_round_number, currentLocation, &num_adjacent_locations);

                // Add to the queue all adjacent unvisited locations
                for (int i = 0; i < num_adjacent_locations; ++i) {
                    PlaceId adjacentLocation = can_go[i];

                    if (visited[adjacentLocation] == BFS_UNVISITED_VALUE) {
                        visited[adjacentLocation] = currentLocation;
                        AddtoQueue(path_queue, adjacentLocation);
                    }
                }

                free(can_go);
            }
	    }

	    ++current_round_number;
	}

	if (!found) {
	    *pathLength = 0;
	    return NULL;	
	}
	
	// Prepare to reconstruct the shortest path
	int i = 0;
	PlaceId loop = dest;
    PlaceId *reverse_path = malloc (sizeof(int) * NUM_REAL_PLACES);
	while (loop != source) {
	    reverse_path[i] = loop;
	    i++;
	    loop = visited[loop];
	}
	*pathLength = i;
	
	PlaceId *_path = malloc (sizeof (int) * NUM_REAL_PLACES);
	// Flip the path so we go from source to destination
	int j = i-1;
	int p = 0;
	while (j >= 0) {
	    _path[p] = reverse_path[j];
	    j--;
	    p++;
	}

	free(reverse_path);
	free(visited);
	FreeQueue(path_queue);

	return _path;
}

////////////////////////////////////////////////////////////////////////
// Making a Move

PlaceId *HvWhereCanIGo(HunterView hv, int *numReturnedLocs) {
    return HvWhereCanTheyGo(hv, HvGetPlayer(hv), numReturnedLocs);
}

PlaceId *HvWhereCanIGoByType(HunterView hv, bool road, bool rail,
                             bool boat, int *numReturnedLocs) {
    return HvWhereCanTheyGoByType(hv, HvGetPlayer(hv), road, rail, boat,
                                  numReturnedLocs);
}

PlaceId *HvWhereCanTheyGo(HunterView hv, Player player,
                          int *numReturnedLocs) {
    if (HvGetPlayerLocation(hv, player) == NOWHERE) {
        *numReturnedLocs = 0;
        return NULL;
    }


    int adjusted_round = HvGetRound(hv) + (player < HvGetPlayer(hv));
    return GvGetReachable(hv->gv, player, adjusted_round, HvGetPlayerLocation(hv, player), numReturnedLocs);
}

PlaceId *HvWhereCanTheyGoByType(HunterView hv, Player player,
                                bool road, bool rail, bool boat,
                                int *numReturnedLocs) {
    if (HvGetPlayerLocation(hv, player) == NOWHERE) {
        *numReturnedLocs = 0;
        return NULL;
    }
    int adjusted_round = HvGetRound(hv) + (player < HvGetPlayer(hv));
    return GvGetReachableByType(hv->gv, player, adjusted_round, HvGetPlayerLocation(hv, player), road, rail, boat, numReturnedLocs);
}

////////////////////////////////////////////////////////////////////////
// Your own interface functions

// TODO




