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
#include "Queue.h"
#include "LocationDynamicArray.h"
#include <stdio.h>


void decideHunterMove(HunterView hv)
{
	// check if hunter knows dracula's location
	Round dracula_last_round = -1;
	Round current_round = HvGetRound(hv);
	PlaceId drac_loc = HvGetLastKnownDraculaLocation(hv,&dracula_last_round);
	Player curr_player = HvGetPlayer(hv); // players 0,1,2,3,4 (typedef enum)
	PlaceId curr_loc = HvGetPlayerLocation(hv,curr_player);
	char *msg = "dummy message";
	
	//fixed moves for round 0, should spread the hunters out well
	if (current_round <= 1) {
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
	} else if (current_round < 6) {//can't reveal end of trail yet
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
	} else if ((current_round - dracula_last_round) >= 12) {
	    PlaceId move = curr_loc;
	    registerBestPlay((char *)placeIdToAbbrev(move), msg);
	    return;
	} else {
	    //number of rounds that have passed since last known dracula location
	    //determined how far he could be now
        Round bfs_cap = current_round - dracula_last_round;
        
        //setting up distance array for every place in game
        int distance_array[71] = {-1};
        
        //setting last known location in distance array = 0
        distance_array[(int)drac_loc] = 0;
        
        //outer for loop determines how far dracula can travel
        for (int i = 1; i <= bfs_cap; i++) {
            //loops through distance array to check for latest places he could be
            for (int j = 0; j < 71; j++) {
                //if a place in the distance array could have been visited in previous
                //round to what we are checking
                if (distance_array[j] < i && distance_array[j] >= 0) {
                    int numReturnedLocs;
                    PlaceId *reachable = HvWhereCanDraculaGoByRound(hv, 
                                         PLAYER_DRACULA, (PlaceId)j,
                                         &numReturnedLocs,
                                         (dracula_last_round + i));
                    
                    //loop through reachable and update distance array                    
                    for (int k = 0; k < numReturnedLocs; k++) {
                        if (distance_array[reachable[k]] < 0) {
                            distance_array[reachable[k]] = i;
                        }
                    }
                    
                }
            }
        
        }
        
	    
	    return;
	}
	
	

}


