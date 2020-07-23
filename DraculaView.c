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
#define usedDB(move) ((move) >= DOUBLE_BACK_1)
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
	int currHist = 0; bool noFree = false;
	int currLoc = currPlace(dv);
	PlaceId *moveHist = GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &currHist, &noFree);

	if (currHist == 0) { 
		*numReturnedMoves = 0;
		return NULL;
	}

	// Following moves: Initialises rPos to track most recent move. 
	// Quickly scans up to 5 prev. moves in MoveHist for HIDE (veiled),
	// DOUBLE_BACK (receded) and whether it leaves next round (DBend). 
	// Performs comparisons between arrays, moveHist and adjLocs, to determine valid moves.
	int numMoves = 0;
	PlaceId *adjLocs = GvGetReachableByType(dv->dracInfo, PLAYER_DRACULA, DvGetRound(dv), 
						currLoc, true, false, true, &numMoves);
	
	int veiled = NO, receded = NO, DBend = NO;
	for (int prev = 0, rP = currHist - 1; rP >= 0 && prev < 5; prev++, rP--) {
		veiled += (moveHist[rP] == HIDE);
		receded += usedDB(moveHist[rP]);
		DBend += (usedDB(moveHist[rP]) && prev == 4);
	}

	int enPt = 0;
	PlaceId *validMoves = malloc(numMoves * sizeof(PlaceId));
	
	for (int i = 0; i < numMoves; i++) {
		int prev = 0, rP = currHist - 1;
		int canDB = NO;
		while (rP >= 0 && prev < 5) {
			canDB += (adjLocs[i] == moveHist[rP] || DBend == YES);
			if (canDB == YES) break;
			prev++, rP--;
		}
		
		if ((DBend == YES || receded == NO) && canDB == YES) {
			validMoves[enPt] = DOUBLE_BACK_1 + prev;
			enPt++;	
		} else if (canDB == NO) {
			validMoves[enPt] = adjLocs[i]; 
			enPt++;
		}
	} 

	if (DBend == YES || receded == NO) {
		validMoves = realloc(validMoves, numMoves++ * sizeof(PlaceId));
		validMoves[enPt] = DOUBLE_BACK_1;
		enPt++;
	}

	if (veiled == NO) {
		validMoves = realloc(validMoves, numMoves++ * sizeof(PlaceId));
		validMoves[enPt] = HIDE;
		enPt++;
	}

	*numReturnedMoves = enPt;
	return validMoves;
}

PlaceId *DvWhereCanIGo(DraculaView dv, int *numReturnedLocs)
{
	// Dracula, at the start, has no move history so return NULL.
	int currHist = 0; bool noFree = false;
	GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &currHist, &noFree);
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
	int currHist = 0; bool noFree = false;
	GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &currHist, &noFree);
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
	int currHist = 0; bool noFree = false;
	GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &currHist, &noFree);
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
	int currHist = 0; bool noFree = false;
	GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &currHist, &noFree);
	if (currHist == 0) { 
		*numReturnedLocs = 0;
		return NULL;
	}

	return GvGetReachableByType(dv->dracInfo, player, DvGetRound(dv), 
    DvGetPlayerLocation(dv, player), road, false, boat, numReturnedLocs);
}

////////////////////////////////////////////////////////////////////////
