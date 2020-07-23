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
#define canDBorHide() (cmp != moveHist[rP] && cmp != currLoc)
#define YES 	1
#define NO 		0

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
	int currHist;
	PlaceId *moveHist = GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &currHist, false);
	if (currHist == 0) { 
		*numReturnedMoves = 0;
		return NULL;
	}

	// Following moves: Initialises rPos to track most recent move. Quickly scans up to
	// 5 prev. moves in MoveHist for HIDE (veiled) and DOUBLE_BACK (receded). Performs
	// comparisons between arrays, moveHist and adjLoc, to determine valid moves.
	int currLoc = currPlace(dv), numMoves;
	PlaceId *adjLoc = GvGetReachableByType(dv->dracInfo, PLAYER_DRACULA, DvGetRound(dv), 
	                    	currLoc, true, false, true, &numMoves);
	
	int veiled = NO, receded = NO;
	for (int prev = 0, rP = currHist - 1; rP >= 0 && prev < 5; prev++, rP--) {
		veiled += (moveHist[rP] == HIDE);
		receded += (moveHist[rP] >= DOUBLE_BACK_1);
	}

	int enPt = 0, *validMoves = malloc(numMoves * sizeof(int));
	for (int i = 0; i < numMoves; i++) {
		int prev = 0, rP = currHist - 1;
		int canHide = (adjLoc[i] == currLoc), canDB = 0;
		while (rP >= 0 && prev < 5) {
			if (canDB + canHide == 1) break;
			if (moveHist[rP] >= HIDE) continue;

			canDB += (moveHist[rP] == adjLoc[i]);
			prev++, rP--;
		}

		if (canDB == NO && canHide == NO) {
			validMoves[enPt] = adjLoc[i];
			enPt++;
		} else if (canDB == YES && receded == NO) {
			validMoves[enPt] = DOUBLE_BACK_1 + prev;
			enPt++;
		} else if (canHide == YES && veiled == NO) {
			validMoves[enPt] = HIDE;
			enPt++;
		}
	}
	
	free(adjLoc);
	if (enPt == 0) {
		*numReturnedMoves = 0;
		free(validMoves);
		return NULL;
	}

	validMoves = realloc(validMoves, enPt * sizeof(int));
	*numReturnedMoves = enPt;
	return adjLoc;
}

PlaceId *DvWhereCanIGo(DraculaView dv, int *numReturnedLocs)
{
	// Dracula, at the start, has no move history so return NULL.
	int currHist;
	PlaceId *moveHist = GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &currHist, false);
	if (currHist == 0) { 
		*numReturnedLocs = 0;
		return NULL;
	}
	
	return DvWhereCanTheyGo(dv, PLAYER_DRACULA, numReturnedLocs);
}

PlaceId *DvWhereCanIGoByType(DraculaView dv, bool road, bool boat,
                             int *numReturnedLocs)
{
	// Dracula, at the start, has no move history so return NULL.
	int currHist;
	PlaceId *moveHist = GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &currHist, false);
	if (currHist == 0) { 
		*numReturnedLocs = 0;
		return NULL;
	}
	return GvGetReachableByType(dv->dracInfo, PLAYER_DRACULA, DvGetRound(dv),
	currPlace(dv), road, false, boat, numReturnedLocs);
}

PlaceId *DvWhereCanTheyGo(DraculaView dv, Player player,
                          int *numReturnedLocs)
{
	// Dracula, at the start, has no move history so return NULL.
	int currHist;
	PlaceId *moveHist = GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &currHist, false);
	if (currHist == 0) { 
		*numReturnedLocs = 0;
		return NULL;
	}
	return GvGetReachable(dv->dracInfo, player, DvGetRound(dv), 
	DvGetPlayerLocation(dv, player), numReturnedLocs);
}

PlaceId *DvWhereCanTheyGoByType(DraculaView dv, Player player,
                                bool road, bool rail, bool boat,
                                int *numReturnedLocs)
{
	// Dracula, at the start, has no move history so return NULL.
	int currHist;
	PlaceId *moveHist = GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &currHist, false);
	if (currHist == 0) { 
		*numReturnedLocs = 0;
		return NULL;
	}

	return GvGetReachableByType(dv->dracInfo, player, DvGetRound(dv), 
    DvGetPlayerLocation(dv, player), road, false, boat, numReturnedLocs);
}

////////////////////////////////////////////////////////////////////////
