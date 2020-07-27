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
	// Dracula, at the start, makes no move; NULL is returned.
	int histLen = 0; bool noFree = false;
	PlaceId *moveHist = GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &histLen, &noFree);
	if (histLen == 0) { 
		*numReturnedMoves = 0;
		return NULL;
	}

	// In terms of usage: [veiled] detects HIDE, [receded] detects DOUBLE_BACK.
	// [rP] tracks most recent move, [prev] limits search, up to 5 moves.
	int veiled = NO, receded = NO;
	for (int prev = 1, rP = histLen - 1; rP >= 0 && prev <= 5; prev++, rP--) {
		veiled += (moveHist[rP] == HIDE);
		receded += (moveHist[rP] >= DOUBLE_BACK_1);
	}

	int numMoves = 0;
	PlaceId *adjLocs = GvGetReachableByType(dv->dracInfo, PLAYER_DRACULA, DvGetRound(dv), 
											currPlace(dv), true, false, true, &numMoves);
	
	int enPt = 0; PlaceId *validMoves = malloc((numMoves + 2) * sizeof(PlaceId));
	if (validMoves == NULL) {
		fprintf(stderr, "Unable to allocate validMoves. Aborting...\n");
            exit(EXIT_FAILURE);
	}

	// Performs array comparsions between moveHist and adjLocs with accord
	// to [veiled], [receded].
	// If adjLoc is unique or found in last 5 rounds with [receded] = NO,
	// add to validMoves. Otherwise, obtain another location from adjLocs.
	for (int i = 0; i < numMoves; i++) {
		int prev = 0, rP = histLen - 1, canDB = NO;
		while (rP >= 0 && prev < 5) {
			canDB += (adjLocs[i] == moveHist[rP]);
			if (canDB == YES) break;
			prev++, rP--;
		}
		
		if (receded == NO && canDB == YES) {
			validMoves[enPt] = DOUBLE_BACK_1 + prev;
			enPt++;	
		} else if (canDB == NO) { 
			validMoves[enPt] = adjLocs[i]; 
			enPt++;
		}
	} 

	if (veiled == NO) {
		validMoves[enPt] = HIDE;
		enPt++;
	}

	if (numMoves == 0) {
		free(validMoves);
		*numReturnedMoves = 0;
		return NULL;
	}

	*numReturnedMoves = enPt;
	return validMoves;
}

PlaceId *FilterDuplicates(DraculaView dv, PlaceId *adjLocs, int *numMoves)
{
	// Dracula, at the start, has no move history so return NULL.
	int histLen = 0; bool noFree = false;
	PlaceId *moveHist = GvGetMoveHistory(dv->dracInfo, PLAYER_DRACULA, &histLen, &noFree);
	
	if (histLen == 0) { 
		*numMoves = 0;
		return NULL;
	}		

	int enPt = 0; PlaceId *validMoves = malloc((*numMoves + 1) * sizeof(PlaceId));
	if (validMoves == NULL) {
		fprintf(stderr, "Unable to allocate validMoves. Aborting...\n");
            exit(EXIT_FAILURE);
	}

	for (int i = 0; i < *numMoves; i++) {
		int prev = 0, rP = histLen - 1, isDup = NO;
		while (rP >= 0 && prev < 5) {
			isDup += (adjLocs[i] == moveHist[rP]);
			if (isDup == YES) break;
			prev++, rP--;
		}
		
		if (isDup == NO) {
			validMoves[enPt] = adjLocs[i]; 
			enPt++;
		}
	} 

	if (enPt == 0) {
		free(validMoves);
		*numMoves = 0;
		return NULL;
	}

	//for(int i = 0; i < enPt; i++)
	//	printf("%d ", validMoves[i]);

	*numMoves = enPt;
	return validMoves;

}

PlaceId *DvWhereCanIGo(DraculaView dv, int *numReturnedLocs)
{
	int numMoves = 0;
	PlaceId *adjLocs = GvGetReachable(dv->dracInfo, PLAYER_DRACULA, DvGetRound(dv), 
						currPlace(dv), &numMoves);

	PlaceId *locationSet = FilterDuplicates(dv,adjLocs, &numMoves);
	*numReturnedLocs = numMoves;

	return locationSet;
	
}

PlaceId *DvWhereCanIGoByType(DraculaView dv, bool road, bool boat,
                             int *numReturnedLocs) {

	int numMoves = 0;
	PlaceId *adjLocs = GvGetReachableByType(dv->dracInfo, PLAYER_DRACULA, DvGetRound(dv), 
						currPlace(dv), road, false, boat, &numMoves);
	
	PlaceId *locationSet = FilterDuplicates(dv,adjLocs, &numMoves);
	*numReturnedLocs = numMoves;
	return locationSet;
}

PlaceId *DvWhereCanTheyGo(DraculaView dv, Player player, int *numReturnedLocs) 
{
	int adjusted_round = DvGetRound(dv) + (player < GvGetPlayer(dv->dracInfo));
	if (player == PLAYER_DRACULA) {
		return DvWhereCanIGo(dv, numReturnedLocs);
	}

	return GvGetReachable(dv->dracInfo, player, adjusted_round, 
				DvGetPlayerLocation(dv, player), numReturnedLocs);
}

PlaceId *DvWhereCanTheyGoByType(DraculaView dv, Player player,
                                bool road, bool rail, bool boat,
                                int *numReturnedLocs) 
{

	int adjusted_round = DvGetRound(dv) + (player < GvGetPlayer(dv->dracInfo));
    if (player == PLAYER_DRACULA) {
		return DvWhereCanIGoByType(dv, road, boat, numReturnedLocs);
	}
	return GvGetReachableByType(dv->dracInfo, player, adjusted_round, 
			DvGetPlayerLocation(dv, player), road, rail, boat, numReturnedLocs);
}

////////////////////////////////////////////////////////////////////////