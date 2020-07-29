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
	Round round;
	PlaceId drac_loc = HvGetLastKnownDraculaLocation(hv,&round);
	Player curr_player = HvGetPlayer(hv); // players 0,1,2,3,4 (typedef enum)
	PlaceId curr_loc = HvGetPlayerLocation(hv,curr_player);
	char *msg = "dummy message";
	PlaceId move;


	// Dracula's location has never been revealed.
	if (drac_loc == NOWHERE){
		printf("Dracula's location has never been revealed\n");
		move = curr_loc;
	}else{
		printf("Dracula's location has been revealed : Location -> %s\n", PLACES[drac_loc].abbrev);
		printf("%d\n",round);
		printf("%s\n", PLACES[HvGetPlayerLocation(hv,curr_player)].name);
		int pathLength;
		PlaceId *path = HvGetShortestPathTo(hv,curr_player,drac_loc,&pathLength);
		move = path[0];
	}
	
	// parse in the best play
	registerBestPlay((char *)placeIdToAbbrev(move),msg);
}


