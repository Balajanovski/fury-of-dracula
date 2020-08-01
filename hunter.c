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
#define INTEG_BARS 1000

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
	if (curr_round <= 1) {
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
	} else if (curr_round < 6) { // Can't reveal end of trail yet
	    if (curr_player == PLAYER_DR_SEWARD) {
	        registerBestPlay("ED", msg);
	        return;
	    } else if (curr_player == PLAYER_LORD_GODALMING) {
	        registerBestPlay("SJ", msg);
	        return;
	    } else if (curr_player == PLAYER_MINA_HARKER) {
	        registerBestPlay("LS", msg);
	        return;
	    } else if (curr_player == PLAYER_VAN_HELSING) {
	        registerBestPlay("AT", msg);
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
		Round bfs_cap = (curr_round - 1) - dracula_last_round;
        
        float distance[NUM_REAL_PLACES];
		for (int i = 0; i < NUM_REAL_PLACES; i++) {
			distance[i] = -1;
		}
        distance[(int)drac_loc] = 0; // Last known location is set to 0
        
        // Outer loop determines how far dracula can travel
        for (int i = 1; i <= bfs_cap; i++) {
            // Inside loops through distance array to check for latest places he could be
            for (int j = 0; j < NUM_REAL_PLACES; j++) {
                // If a place in the distance array could have been visited in previous
                // round to what we are checking
                if (distance[j] == i - 1) {
					int numReturnedLocs;
                    PlaceId *reachable = HvWhereCanDraculaGoByRound(hv, 
                                         PLAYER_DRACULA, (PlaceId)j,
                                         &numReturnedLocs,
                                         (dracula_last_round + i));
                    
                    // Loop through reachable and update distance array                    
                    for (int k = 0; k < numReturnedLocs; k++) {
                        if (distance[reachable[k]] < 0) {
                            distance[reachable[k]] = i;
                        }
                    }
                    
                }
            }
		}
		
		// With elements ≥ 0 in the distance_array, a mean and
		// standard deviation, variance can be inferred upon.
		double mean = findMean(distance, NUM_REAL_PLACES);
		double variance = findVariance(distance, mean, NUM_REAL_PLACES);
		double STDdev = findSTDdeviation(variance);

		// Combined, gaussian density probabilities are calculated for each radius
		for (int i = 1; i <= bfs_cap; i++) {
			float prob = getRadiusProbability(i - 1, i, mean, variance, STDdev);

			// Calculated probability is corresponded to the radius set by distance[]
			for (int j = 0; j < NUM_REAL_PLACES; j++) {
				if (distance[j] == i) distance[j] = prob;
			}
		}

		// Debugging tool: Distance is filled now filled with probabilities
		// corresponding to their radius
		//for (int i = 0; i < NUM_REAL_PLACES; i++)
		//	printf("[%d, %lf] ", i, distance[i]);
	}
	
}