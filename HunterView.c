////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// HunterView.c: the HunterView ADT implementation
//
// 2014-07-01	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2017-12-01	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v2.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10   v3.0    Team Dracula <cs2521@cse.unsw.edu.au>
//
////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "Game.h"
#include "GameView.h"
#include "HunterView.h"
#include "Map.h"
#include "Places.h"
#include "Queue.h"
// add your own #includes here

// TODO: ADD YOUR OWN STRUCTS HERE

struct hunterView {
    GameView gv;

};

////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

HunterView HvNew(char *pastPlays, Message messages[])
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	HunterView new = malloc(sizeof(*new));
	if (new == NULL) {
		fprintf(stderr, "Couldn't allocate HunterView!\n");
		exit(EXIT_FAILURE);
	}
	new->gv = GvNew(pastPlays, messages);
	

	return new;
}

void HvFree(HunterView hv)
{
    assert(hv != NULL);
    GvFree(hv->gv);
	free(hv);
}

////////////////////////////////////////////////////////////////////////
// Game State Information

Round HvGetRound(HunterView hv)
{
	assert (hv != NULL);
	return GvGetRound(hv->gv);
}


Player HvGetPlayer(HunterView hv)
{
	assert (hv != NULL);
	return GvGetPlayer(hv->gv);
}

int HvGetScore(HunterView hv)
{
	assert (hv != NULL);
	return GvGetScore(hv->gv);
}

int HvGetHealth(HunterView hv, Player player)
{

	assert (hv != NULL);
	return GvGetHealth(hv->gv, player);
}

PlaceId HvGetPlayerLocation(HunterView hv, Player player)
{
    assert (hv != NULL);
    return GvGetPlayerLocation(hv->gv, player);

}

PlaceId HvGetVampireLocation(HunterView hv)
{
	assert (hv != NULL);
	return GvGetVampireLocation(hv->gv);
}

////////////////////////////////////////////////////////////////////////
// Utility Functions

PlaceId HvGetLastKnownDraculaLocation(HunterView hv, Round *round)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	*round = 0;
	return NOWHERE;
}

PlaceId *HvGetShortestPathTo(HunterView hv, Player hunter, PlaceId dest,
                             int *pathLength)
{
	// TODO: REPLACE THIS WITH YOUR OWN IMPLEMENTATION
	Queue Path = NewQueue();
	
    //set up visited array
    //71 places in map
	int *visited = malloc (sizeof (int)*71);
	
	
	
	//set visited array to -1 for all
	for (int i = 0; i < 71; i++) {
	    visited[i] = -1;
	
	}
	
	//get start location
	PlaceId source = GvGetPlayerLocation(hv->gv, hunter);
	
	//set source to visited
	visited[source] = source;
	
	//add source to queue
	AddtoQueue(Path, source);
	
	int found = 0;
	
	while (found == 0 && QueueSize(Path) != 0) {
	    PlaceId v = RemovefromQueue(Path);
	    printf("%d\n", v);
	    //if found destination end loop
	    if (v == dest) {
	        found = 1;
	        break;
	    } else {
	        //Get array of places hunter can go
	        int numberCanGo;
	        PlaceId *canGo = //HvWhereCanIGo(hv, &numberCanGo);
	        GvGetReachable(hv->gv, GvGetPlayer(hv->gv), GvGetRound(hv->gv), GvGetPlayerLocation(hv->gv, GvGetPlayer(hv->gv)), &numberCanGo);
	        //loop through array of cango and add to queue if not visited
	        for (int i = 0; i < numberCanGo; i++) {
	            PlaceId w = canGo[i];
	            if (visited[w] == -1) {
	                visited[w] = v;
	                AddtoQueue(Path, w);
	            }
	        }  
	    }
	}
	
	//if no path return 0
	if (found == 0) {
	    *pathLength = 0;
	    return NULL;	
	}
	
	//allocate reverse path
	int i = 0;
	PlaceId loop = dest;
    PlaceId *reverse_path = malloc (sizeof (int)*71);
	while (loop != source) {
	    reverse_path[i] = loop;
	    i++;
	    loop = visited[loop];
	}
	
	reverse_path[i] = loop;
	*pathLength = i + 1;
	
	PlaceId *_path = malloc (sizeof (int)*71);
	//flip the path so we go from source to destination
	int j = i;
	int p = 0;
	while (j >= 0) {
	    _path[p] = reverse_path[j];
	    j--;
	    p++;
	}
	
	return _path;

}

////////////////////////////////////////////////////////////////////////
// Making a Move

PlaceId *HvWhereCanIGo(HunterView hv, int *numReturnedLocs)
{	
    //if player hasn't gone yet should return NULL
    //so have to check if player has a move history
    
    if (GvGetRound(hv->gv) == 0) {
        numReturnedLocs = 0;
        return NULL;
    } else {
        return HvWhereCanTheyGo(hv, HvGetPlayer(hv), numReturnedLocs);
    }
}

PlaceId *HvWhereCanIGoByType(HunterView hv, bool road, bool rail,
                             bool boat, int *numReturnedLocs)
{
    //if player hasn't gone yet should return NULL
    //so have to check if player has a move history
    
    if (GvGetRound(hv->gv) == 0) {
        numReturnedLocs = 0;
        return NULL;
    } else {
        return HvWhereCanTheyGoByType(hv, HvGetPlayer(hv), road, rail, boat,
                                      numReturnedLocs);
    }
}

PlaceId *HvWhereCanTheyGo(HunterView hv, Player player,
                          int *numReturnedLocs)
{
    
    if (GvGetRound(hv->gv) == 0) {
        numReturnedLocs = 0;
        return NULL;
    } else {
    return GvGetReachable(hv->gv, player, HvGetRound(hv),
     HvGetPlayerLocation(hv, player), numReturnedLocs);
    }

}

PlaceId *HvWhereCanTheyGoByType(HunterView hv, Player player,
                                bool road, bool rail, bool boat,
                                int *numReturnedLocs)
{

    
    /*if (GvGetRound(hv->gv) == 0) {
        numReturnedLocs = 0;

        return NULL;
    } else {*/
    return GvGetReachableByType(hv->gv, player, HvGetRound(hv), 
    HvGetPlayerLocation(hv, player), road, rail, boat, numReturnedLocs);
    //}
}

////////////////////////////////////////////////////////////////////////
// Your own interface functions

// TODO




