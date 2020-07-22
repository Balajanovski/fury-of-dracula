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

#include "LocationDynamicArray.h"
#include "DraculaView.h"
#include "Game.h"
#include "GameView.h"
#include "Map.h"
// add your own #includes here
#define currPlace(gv) (DvGetPlayerLocation(gv, PLAYER_DRACULA))

struct draculaView {
	GameView dracInfo;
};

////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

DraculaView DvNew(char *pastPlays, Message messages[])
{
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
	return GvGetVampireLocation(dv->dracInfo);
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
	// Dracula, at the start, has no move history so return NULL.
	int numMoves = 0;
	PlaceId *moveHist = GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &numMoves, false);
	if (numMoves == 0) { 
		*numReturnedMoves = 0;
		return NULL;
	}

	// Following moves: Initialises rPos to track most recent move. Analyses 
	// prev. 5 moves and check for HIDE (veiled) and DOUBLE_BACK (receded).
	// If both moves are used, return an array of locations. Otherwise, perform
	// comparisons between arrays, moveHist and locSet, to determine valid moves.
	int rPos = numMoves - 1, rP = rPos, veiled = 0, receded = 0;
	for (int prevR = 0; rPos >= 0 && prevR < 5; prevR++) {
		veiled = (moveHist[rPos] == HIDE);
		receded = (moveHist[rPos] >= DOUBLE_BACK_1);
		rPos--;
	}
	
	PlaceId *locSet = GvGetReachableByType(dv->dracInfo, PLAYER_DRACULA, DvGetRound(dv), 
	                    	currPlace(dv), true, false, true, &numMoves);
	
	//HIDE and DOUBLE_BACK, are used, return location moves.
	if (veiled * receded == 1) {
		*numReturnedMoves = numMoves;
		return locSet;
	}

	int currLoc = currPlace(dv);
	int enPt = 0,*validMoves = malloc(numMoves * sizeof(int));
	for (int prevR = 0; rP >= 0 && prevR < 5; prevR++) {
		for (int j = 0; j < numMoves; j++) {
			if (receded == 0 && locSet[j] == moveHist[rP]) {
				validMoves[enPt] = DOUBLE_BACK_1 + prevR;
				enPt++;
				break;
			} else if (veiled == 0 && locSet[j] == currLoc) { 
				validMoves[enPt] = HIDE;
				enPt++;
				break;
			}
		}
		rP--;
	}

	*numReturnedMoves = numMoves;
	return locSet;
}

PlaceId *DvWhereCanIGo(DraculaView dv, int *numReturnedLocs)
{
	return DvWhereCanTheyGo(dv, PLAYER_DRACULA, numReturnedLocs);
}

PlaceId *DvWhereCanIGoByType(DraculaView dv, bool road, bool boat,
                             int *numReturnedLocs)
{
	return GvGetReachableByType(dv->dracInfo, PLAYER_DRACULA, DvGetRound(dv),
	currPlace(dv), road, false, boat, numReturnedLocs);
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
