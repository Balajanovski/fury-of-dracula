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

#include "DraculaTrail.h"
#include "DraculaView.h"
#include "Game.h"
#include "GameView.h"
#include "Map.h"
// add your own #includes here

// TODO: ADD YOUR OWN STRUCTS HERE

struct draculaView {
	GameView dracInfo;
};

////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

DraculaView DvNew(char *pastPlays, Message messages[])
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	DraculaView new = malloc(sizeof(*new));
	if (new == NULL) {
		fprintf(stderr, "Couldn't allocate DraculaView\n");
		exit(EXIT_FAILURE);
	}

	new->dracInfo = GvNew(pastPlays, messages);

	return new;
}

void DvFree(DraculaView dv)
{
	assert(dv != NULL);
	GvFree(dv->dracInfo);
	free(dv);
}

////////////////////////////////////////////////////////////////////////
// Game State Information

Round DvGetRound(DraculaView dv)
{
	assert(dv != NULL);
	return GvGetRound(dv->dracInfo);
}

int DvGetScore(DraculaView dv)
{
	assert(dv != NULL);
	return GvGetScore(dv->dracInfo);
}

int DvGetHealth(DraculaView dv, Player player)
{	
	assert(dv != NULL);
	return GvGetHealth(dv->dracInfo, player);
}

PlaceId DvGetPlayerLocation(DraculaView dv, Player player)
{
	assert(dv != NULL);
	return GvGetPlayerLocation(dv->dracInfo, player);
}

PlaceId DvGetVampireLocation(DraculaView dv)
{
	assert(dv != NULL);
	return GvGetDracula(dv->dracInfo);
}

PlaceId *DvGetTrapLocations(DraculaView dv, int *numTraps)
{
	assert(dv != NULL);
	assert(numTraps != NULL);
	return GvGetTrapLocations(dv->dracInfo, numTraps);
}

////////////////////////////////////////////////////////////////////////
// Making a Move

PlaceId *DvGetValidMoves(DraculaView dv, int *numReturnedMoves)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	return NOWHERE;
}

PlaceId *DvWhereCanIGo(DraculaView dv, int *numReturnedLocs)
{
	return DvWhereCanTheyGo(dv->dracInfo, PLAYER_DRACULA, numReturnedLocs);
}

PlaceId *DvWhereCanIGoByType(DraculaView dv, bool road, bool boat,
                             int *numReturnedLocs)
{
	return GvGetReachableByType(dv->dracInfo, PLAYER_DRACULA, DvGetRound(dv),
	DvGetPlayer(dv), road, false, boat, numReturnedLocs);
}

PlaceId *DvWhereCanTheyGo(DraculaView dv, Player player,
                          int *numReturnedLocs)
{
	return GvGetReachable(dv->dracInfo, player, DvGetRound(dv), 
	DvGetPlayerLocation(dv, player), numReturnedLocs);
}

PlaceId *DvWhereCanTheyGoByType(DraculaView dv, Player player,
                                bool road, bool rail, bool boat,
                                int *numReturnedLocs)
{

	return GvGetReachableByType(dv->dracInfo, player, DvGetRound(dv), 
    DvGetPlayerLocation(dv, player), road, false, boat, numReturnedLocs);
}

////////////////////////////////////////////////////////////////////////
