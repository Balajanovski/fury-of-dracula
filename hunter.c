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
	    //breadth first search to determine probability of where dracula is
	    return;
	}
	
	

}


