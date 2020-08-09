////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// hunter.c: your "Fury of Dracula" hunter AI.
//
// 2014-07-01	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2017-12-01	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v2.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10	v3.0	Team Dracula <cs2521@cse.unsw.edu.au>
//
////////////////////////////////////////////////////////////////////////
// STAGE
// 2020-07-29	Hunters will be moving to lastknowndraculalocation if possible
///////////////////////////////////////////////////////////////////////

#include "Game.h"
#include "hunter.h"
#include "HunterView.h"
#include "Probability.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "Queue.h"
#include "Places.h"

void decideHunterMove(HunterView hv)
{
	// Check if hunter knows dracula's location
	Round dracula_last_round = -1;
	PlaceId drac_loc = HvGetLastKnownDraculaLocation(hv,&dracula_last_round);

	// Get game info about the present round
	Round curr_round = HvGetRound(hv);
	Player curr_player = HvGetPlayer(hv); // players 0,1,2,3,4 (typedef enum)
	PlaceId curr_loc = HvGetPlayerLocation(hv,curr_player);
	char *msg = "Your bloodsucking days end now.";
	
	// Get location occupied by other hunters
	PlaceId occupied[3], enPt = 0;
	for (int i = 0; i < NUM_PLAYERS - 1; i++) {
		if (i == curr_player) continue;
		occupied[enPt] = HvGetPlayerLocation(hv,i);
		enPt++;
	}

	if (curr_round < 1) { // Fixed moves for round 0; hunter locations are spread
		switch (curr_player) 
		{
		    case PLAYER_DR_SEWARD :
				registerBestPlay("CD", msg); break;
		    case PLAYER_LORD_GODALMING : 
				registerBestPlay("MA", msg); break;
		    case PLAYER_MINA_HARKER :
		        registerBestPlay("CO", msg); break;
		    case PLAYER_VAN_HELSING : 
		        registerBestPlay("MI", msg); break;
		    default :
		        printf("Invalid player");
		}
		return;
	} else if (curr_round < 6 && drac_loc == NOWHERE) { // Can't reveal end of trail yet
		PlaceId loc = -1;
		switch (curr_player) 
		{
		    case PLAYER_DR_SEWARD : 
				loc = LONDON; break;
		    case PLAYER_LORD_GODALMING : 
				loc = CASTLE_DRACULA; break;
		    case PLAYER_MINA_HARKER :
		        loc = LISBON; break;
		    case PLAYER_VAN_HELSING : 
		        loc = VALONA; break;
		    default :
		        printf("Invalid player");
		}

		int pathLen = 0;
		PlaceId *shortestPath = HvGetShortestPathTo(hv, curr_player, loc, &pathLen);
		PlaceId move = shortestPath[0];
		
		// Ensures chosen move is valid, otherwise select other valid moves
		int valid = 0, numValid;
		PlaceId *validMoves = HvWhereCanIGo(hv, &numValid);
		for (int i = 0; i < numValid; i++) {
		    if (validMoves[i] == shortestPath[0]) {
		        valid = 1;
		    }  
		}
		if (valid == 0) {
		    move = validMoves[curr_round % numValid];
		}
	
		registerBestPlay((char *)placeIdToAbbrev(move), msg);
		return;
	} else if (drac_loc == NOWHERE || HvGetHealth(hv, curr_player) <= 3) { 
	    PlaceId move = curr_loc;
	    registerBestPlay((char *)placeIdToAbbrev(move), msg);
		return;
	}

	// bfs_cap observes if radius probabilities are certain enough for
	// for hunters to search for Dracula or in need of recalculating
	Round bfs_cap = curr_round - dracula_last_round - 1;
	 
	PlaceId move, *path_to_dracula;
	if (bfs_cap <= 0) { // High certainty: Dracula's location was found in the last round
		int PathLen;
		move = curr_loc;
		path_to_dracula = HvGetShortestPathTo(hv, curr_player, drac_loc, &PathLen);
		
		if (PathLen != 0) move = path_to_dracula[0];

		free(path_to_dracula);
		
		int valid = 0, numValid;
		PlaceId *validMoves = HvWhereCanIGo(hv, &numValid);
		for (int i = 0; i < numValid; i++) {
		    if (validMoves[i] == move) {
		        valid = 1;
		    }  
		}

		if (valid == 0) move = validMoves[curr_round % numValid];
		
		registerBestPlay((char *)placeIdToAbbrev(move), msg);
		return;		
	} else if (bfs_cap == 9) { // Low certainty: Probability of his whereabouts is spreading
		move = curr_loc;
		registerBestPlay((char *)placeIdToAbbrev(move), msg);
		return;
	}
	
	// dist_prob[] is used to determine radius of movement according
	// to bfs_cap (max dist. dracula could travel due to round changes)
	float dist_prob[NUM_REAL_PLACES];
	for (int i = 0; i < NUM_REAL_PLACES; i++) dist_prob[i] = -1;

	dist_prob[(int)drac_loc] = 0;
	
	for (int i = 1; i <= bfs_cap; i++) {
		// Inside loops through dist_prob array to check for latest places he could be
		for (int j = 0; j < NUM_REAL_PLACES; j++) {
			// If a place in the dist_prob array could have been visited in previous
			// round to what we are checking
			if (dist_prob[j] == i - 1) {
				int numReturnedLocs;
				PlaceId *reachable = HvWhereCanDraculaGoByRound(hv, PLAYER_DRACULA, (PlaceId)j,
										&numReturnedLocs, (dracula_last_round + i));
				
				// Loop through reachable and update dist_prob array                    
				for (int k = 0; k < numReturnedLocs; k++) {
					if (dist_prob[reachable[k]] < 0) {
						dist_prob[reachable[k]] = i;
					}
				}
				
			}
		}
	}
	
	// With the positive elements inside dist_prob[], mean,
	// standard deviation, variance can be inferred upon.
	double mean = findMean(dist_prob, NUM_REAL_PLACES);
	double variance = findVariance(dist_prob, mean, NUM_REAL_PLACES);
	double STDdev = findSTDdeviation(variance);

	// Gaussian probability is calculated for a radius. It is
	// then matched with locations assigned to that radius.
	float highestProb = 0;
	for (int i = 1; i <= bfs_cap; i++) {
		float prob = getRadiusProbability(i - 1, i, mean, variance, STDdev);

		if (prob > highestProb) highestProb = prob;

		for (int j = 0; j < NUM_REAL_PLACES; j++) {
			if (dist_prob[j] == i) dist_prob[j] = prob;
		}
	}

	// Checks hunter's ability to reach probable locations within their vicinity.
	// Locations not their current one or occupied by other hunters are allowed.
	float mostProb = 0; int numPlaces = -1;
	PlaceId *canGo = HvWhereCanIGo(hv, &numPlaces), max = canGo[0];
	for (int i = 1; i < numPlaces; i++) {		

		int isOccup = 0;
		for (int j = 0; j < 3; j++) {
			if (occupied[j] == canGo[i]) isOccup = 1;
		}

		if (isOccup == 1) continue;
		if (canGo[i] == curr_loc) continue;
		
		if (mostProb < dist_prob[canGo[i]]) {
			mostProb = dist_prob[canGo[i]];
			max = canGo[i];
		}
	}

	// Move hunters closer to the higher probability locations in the
	// shortest possible way if they weren't any 'probable move'.
	// Locations not their current one or occupied by other hunters are allowed.
	if (dist_prob[max] < 0) { 
		int pathLen = -1, shortestLen = 999;
		for (int i = 0; i < NUM_REAL_PLACES; i++) {
			if (dist_prob[i] == highestProb && i != curr_loc) {
				PlaceId *shortest = HvGetShortestPathTo(hv,curr_player,(PlaceId)i,&pathLen);
				if (pathLen < shortestLen && pathLen >= 3) {
					shortestLen = pathLen;
					max = shortest[0];
				}
			}
		}	
	}
	
	int valid = 0, numValid;
	PlaceId *validMoves = HvWhereCanIGo(hv, &numValid);
	for (int i = 0; i < numValid; i++) {
		if (validMoves[i] == max) {
		    valid = 1;
		}  
	}
		
	if (valid == 0) max = validMoves[curr_round % numValid];


	registerBestPlay((char *)placeIdToAbbrev(max), msg);

}
