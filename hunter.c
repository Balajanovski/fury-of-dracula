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
	char *msg = "dummy message";

	// Fixed moves for round 0, should spread the hunters out well
	if (curr_round < 1) {
	    if (curr_player == PLAYER_DR_SEWARD) {
	        registerBestPlay("CD", msg);
	        return;
	    } else if (curr_player == PLAYER_LORD_GODALMING) {
	        registerBestPlay("MA", msg);
	        return;
	    } else if (curr_player == PLAYER_MINA_HARKER) {
	        registerBestPlay("CO", msg);
	        return;
	    } else if (curr_player == PLAYER_VAN_HELSING) {
	        registerBestPlay("MI", msg);
	        return;
	    }
	} else if (curr_round < 6 && drac_loc == NOWHERE) { // Can't reveal end of trail yet
	    if (curr_player == PLAYER_DR_SEWARD) {
	        int pathlen = 0;
	        PlaceId *shortestPath = HvGetShortestPathTo(hv, PLAYER_DR_SEWARD, 
	                                EDINBURGH, &pathlen); 
	        PlaceId move = shortestPath[0];                
	        registerBestPlay((char *)placeIdToAbbrev(move), msg);
	        return;
	    } else if (curr_player == PLAYER_LORD_GODALMING) {
	        int pathlen = 0;
	        PlaceId *shortestPath = HvGetShortestPathTo(hv, PLAYER_DR_SEWARD, 
	                                CASTLE_DRACULA, &pathlen); 
	        PlaceId move = shortestPath[0];                
	        registerBestPlay((char *)placeIdToAbbrev(move), msg);
	        return;
	    } else if (curr_player == PLAYER_MINA_HARKER) {
	        int pathlen = 0;
	        PlaceId *shortestPath = HvGetShortestPathTo(hv, PLAYER_DR_SEWARD, 
	                                LISBON, &pathlen); 
	        PlaceId move = shortestPath[0];                
	        registerBestPlay((char *)placeIdToAbbrev(move), msg);
	        return;
	    } else if (curr_player == PLAYER_VAN_HELSING) {
            int pathlen = 0;
	        PlaceId *shortestPath = HvGetShortestPathTo(hv, PLAYER_DR_SEWARD, 
	                                ATHENS, &pathlen); 
	        PlaceId move = shortestPath[0];                
	        registerBestPlay((char *)placeIdToAbbrev(move), msg);
	        return;
	    }
	
	} else if (drac_loc == NOWHERE) { 
	    PlaceId move = curr_loc;
	    registerBestPlay((char *)placeIdToAbbrev(move), msg);
        return;
	} else if ((curr_round - dracula_last_round) >= 12) {
	    PlaceId move = curr_loc;
	    registerBestPlay((char *)placeIdToAbbrev(move), msg);
	    return;
	} else {
		// Calculates a radius of his whereabouts according to the number
		// of rounds that have passed since last known dracula location.
		Round bfs_cap = curr_round - dracula_last_round - 1;

        float dist_prob[NUM_REAL_PLACES];
		for (int i = 0; i < NUM_REAL_PLACES; i++) dist_prob[i] = -1;
        
		// Dracula's location was found just last round, probability is certain
		// Otherwise, dist_prob[] is 0, set for gaussian calculation
		if (bfs_cap == 0) {
			dist_prob[(int)drac_loc] = 1;
			int PathLen;
        	PlaceId *path_to_dracula = HvGetShortestPathTo(hv, HvGetPlayer(hv), 
                                   drac_loc, &PathLen);
            if (PathLen == 0) {
                registerBestPlay((char *)placeIdToAbbrev(drac_loc), msg);
                return;
            }
        	registerBestPlay((char *)placeIdToAbbrev(path_to_dracula[0]), msg);
	    	return;
		} else {
			dist_prob[(int)drac_loc] = 0;
		}
		
		// Outer loop determines how far dracula can travel
        for (int i = 1; i <= bfs_cap; i++) {
            // Inside loops through dist_prob array to check for latest places he could be
            for (int j = 0; j < NUM_REAL_PLACES; j++) {
                // If a place in the dist_prob array could have been visited in previous
                // round to what we are checking
                if (dist_prob[j] == i - 1) {
					int numReturnedLocs;
                    PlaceId *reachable = HvWhereCanDraculaGoByRound(hv, 
                                         PLAYER_DRACULA, (PlaceId)j,
                                         &numReturnedLocs,
                                         (dracula_last_round + i));
                    
                    // Loop through reachable and update dist_prob array                    
                    for (int k = 0; k < numReturnedLocs; k++) {
                        if (dist_prob[reachable[k]] < 0) {
                            dist_prob[reachable[k]] = i;
                        }
                    }
                    
                }
            }
		}
		
		// With elements ≥ 0 in the dist_prob_array, a mean and
		// standard deviation, variance can be inferred upon.
		double mean = findMean(dist_prob, NUM_REAL_PLACES);
		double variance = findVariance(dist_prob, mean, NUM_REAL_PLACES);
		double STDdev = findSTDdeviation(variance);

		// A gaussian probability is calculated for a radius. It is
		// then matched with locations assigned to that radius.
		float highestProb = 0;
		for (int i = 1; i <= bfs_cap; i++) {
			float prob = getRadiusProbability(i - 1, i, mean, variance, STDdev);
			if (prob > highestProb) highestProb = prob;

			for (int j = 0; j < NUM_REAL_PLACES; j++) {
				if (dist_prob[j] == i) dist_prob[j] = prob;
			}
		}

		// Checks if hunter can reach most probable location in 1 move
		int numPlaces = -1;
		PlaceId *canGo = HvWhereCanIGo(hv, &numPlaces), max = canGo[0];
		for (int i = 0; i < numPlaces; i++) {
		    if (dist_prob[canGo[i]] == highestProb) max = canGo[i];
		}
		//if player can reach highest probability in 1 move,
		//find shortest path to highest probablity and move.
		if(dist_prob[max] < 0){
			int pathlength = 0;
			int shortestlength = 1;
			for(int i = 0; i < NUM_REAL_PLACES; i++){
				if(dist_prob[i] >= highestProb){
					if(shortestlength > pathlength){
					PlaceId *shortest = HvGetShortestPathTo(hv,curr_player,(PlaceId)i,&pathlength);	
					shortestlength = pathlength;
					max = shortest[0];
					}
				}
			}		
		}
		PlaceId move = max;
		registerBestPlay((char *)placeIdToAbbrev(move), msg);
	    return;

	}
	
}