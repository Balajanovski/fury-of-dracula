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

#include "Game.h"
#include "GameView.h"
#include "HunterView.h"
#include "Map.h"
#include "Places.h"
// add your own #includes here

// TODO: ADD YOUR OWN STRUCTS HERE

struct hunterView {
    GameView gv;

};

////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

HunterView HvNew(char *pastPlays, Message messages[])
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	HunterView new = malloc(sizeof(*new));
	if (new == NULL) {
		fprintf(stderr, "Couldn't allocate HunterView!\n");
		exit(EXIT_FAILURE);
	}
	new->gv = GvNew(pastPlays, messages);
	

	return new;
}

void HvFree(HunterView hv)
{
    assert(hv != NULL);
    GvFree(hv->gv);
	free(hv);
}

////////////////////////////////////////////////////////////////////////
// Game State Information

Round HvGetRound(HunterView hv)
{
	assert (hv != NULL);
	return GvGetRound(hv->gv);
}


Player HvGetPlayer(HunterView hv)
{
	assert (hv != NULL);
	return GvGetPlayer(hv->gv);
}

int HvGetScore(HunterView hv)
{
	assert (hv != NULL);
	return GvGetScore(hv->gv);
}

int HvGetHealth(HunterView hv, Player player)
{

	assert (hv != NULL);
	return GvGetHealth(hv->gv, player);
}

PlaceId HvGetPlayerLocation(HunterView hv, Player player)
{
    assert (hv != NULL);
    return GvGetPlayerLocation(hv->gv, player);

}

PlaceId HvGetVampireLocation(HunterView hv)
{
	assert (hv != NULL);
	return GvGetVampireLocation(hv->gv);
}

////////////////////////////////////////////////////////////////////////
// Utility Functions

PlaceId HvGetLastKnownDraculaLocation(HunterView hv, Round *round)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*round = 0;
	return NOWHERE;
}

PlaceId *HvGetShortestPathTo(HunterView hv, Player hunter, PlaceId dest,
                             int *pathLength)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*pathLength = 0;
	return NULL;
}

////////////////////////////////////////////////////////////////////////
// Making a Move

PlaceId *HvWhereCanIGo(HunterView hv, int *numReturnedLocs)
{	
    return HvWhereCanTheyGo(hv, HvGetPlayer(hv), numReturnedLocs);
}

PlaceId *HvWhereCanIGoByType(HunterView hv, bool road, bool rail,
                             bool boat, int *numReturnedLocs)
{
    return HvWhereCanTheyGoByType(hv, HvGetPlayer(hv), road, rail, boat,
                                  numReturnedLocs);
}

PlaceId *HvWhereCanTheyGo(HunterView hv, Player player,
                          int *numReturnedLocs)
{
    return GvGetReachable(hv->gv, player, HvGetRound(hv),
     HvGetPlayerLocation(hv, player), numReturnedLocs);

}

PlaceId *HvWhereCanTheyGoByType(HunterView hv, Player player,
                                bool road, bool rail, bool boat,
                                int *numReturnedLocs)
{
    return GvGetReachableByType(hv->gv, player, HvGetRound(hv), 
    HvGetPlayerLocation(hv, player), road, rail, boat, numReturnedLocs);
}

////////////////////////////////////////////////////////////////////////
// Your own interface functions

// TODO




