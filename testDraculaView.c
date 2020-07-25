////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// testDraculaView.c: test the DraculaView ADT
//
// As supplied, these are very simple tests.  You should write more!
// Don't forget to be rigorous and thorough while writing tests.
//
// 2014-07-01	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2017-12-02	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v2.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10	v3.0	Team Dracula <cs2521@cse.unsw.edu.au>
//
////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DraculaView.h"
#include "Game.h"
#include "Places.h"
#include "testUtils.h"

int main(void)
{
	{///////////////////////////////////////////////////////////////////
	
		printf("Test for basic functions, "
			   "just before Dracula's first move\n");

		char *trail =
			"GST.... SAO.... HZU.... MBB....";
		
		Message messages[] = {
			"Hello", "Goodbye", "Stuff", "..."
		};
		
		DraculaView dv = DvNew(trail, messages);

		assert(DvGetRound(dv) == 0);
		assert(DvGetScore(dv) == GAME_START_SCORE);
		assert(DvGetHealth(dv, PLAYER_DRACULA) == GAME_START_BLOOD_POINTS);
		assert(DvGetPlayerLocation(dv, PLAYER_LORD_GODALMING) == STRASBOURG);
		assert(DvGetPlayerLocation(dv, PLAYER_DR_SEWARD) == ATLANTIC_OCEAN);
		assert(DvGetPlayerLocation(dv, PLAYER_VAN_HELSING) == ZURICH);
		assert(DvGetPlayerLocation(dv, PLAYER_MINA_HARKER) == BAY_OF_BISCAY);
		assert(DvGetPlayerLocation(dv, PLAYER_DRACULA) == NOWHERE);
		assert(DvGetVampireLocation(dv) == NOWHERE);
		int numTraps = -1;
		PlaceId *traps = DvGetTrapLocations(dv, &numTraps);
		assert(numTraps == 0);
		free(traps);

		printf("Test passed!\n");
		DvFree(dv);
	}

	{///////////////////////////////////////////////////////////////////
	
		printf("Test for encountering Dracula\n");

		char *trail =
			"GST.... SAO.... HCD.... MAO.... DGE.V.. "
			"GGEVD.. SAO.... HCD.... MAO....";
		
		Message messages[] = {
			"Hello", "Goodbye", "Stuff", "...", "Mwahahah",
			"Aha!", "", "", ""
		};
		
		DraculaView dv = DvNew(trail, messages);

		assert(DvGetRound(dv) == 1);
		assert(DvGetScore(dv) == GAME_START_SCORE - SCORE_LOSS_DRACULA_TURN);
		assert(DvGetHealth(dv, PLAYER_LORD_GODALMING) == 5);
		assert(DvGetHealth(dv, PLAYER_DRACULA) == 30);
		assert(DvGetPlayerLocation(dv, PLAYER_LORD_GODALMING) == GENEVA);
		assert(DvGetPlayerLocation(dv, PLAYER_DRACULA) == GENEVA);
		assert(DvGetVampireLocation(dv) == NOWHERE);

		printf("Test passed!\n");
		DvFree(dv);
	}

	{///////////////////////////////////////////////////////////////////
	
		printf("Test for Dracula leaving minions 1\n");

		char *trail =
			"GGE.... SGE.... HGE.... MGE.... DED.V.. "
			"GST.... SST.... HST.... MST.... DMNT... "
			"GST.... SST.... HST.... MST.... DLOT... "
			"GST.... SST.... HST.... MST.... DHIT... "
			"GST.... SST.... HST.... MST....";
		
		Message messages[24] = {};
		DraculaView dv = DvNew(trail, messages);

		assert(DvGetRound(dv) == 4);
		assert(DvGetVampireLocation(dv) == EDINBURGH);
		int numTraps = -1;
		PlaceId *traps = DvGetTrapLocations(dv, &numTraps);
		assert(numTraps == 3);
		sortPlaces(traps, numTraps);
		assert(traps[0] == LONDON);
		assert(traps[1] == LONDON);
		assert(traps[2] == MANCHESTER);
		free(traps);
		
		printf("Test passed!\n");
		DvFree(dv);
	}

	{///////////////////////////////////////////////////////////////////
	
		printf("DvGetValidMoves Test #1: General\n");
		
		char *trail =
			"GGE.... SGE.... HGE.... MGE.... DCD.V.. "
			"GGE.... SGE.... HGE.... MGE....";
		
		Message messages[9] = {};
		DraculaView dv = DvNew(trail, messages);
		
		int numMoves = -1;
		PlaceId *moves = DvGetValidMoves(dv, &numMoves);
		assert(numMoves == 4);
		sortPlaces(moves, numMoves);
		assert(moves[0] == GALATZ);
		assert(moves[1] == KLAUSENBURG);
		assert(moves[2] == HIDE);
		assert(moves[3] == DOUBLE_BACK_1);
		free(moves);
		
		printf("Test passed!\n");
		DvFree(dv);
	}
	
	{///////////////////////////////////////////////////////////////////
	
		printf("DvGetValidMoves Test #2: Many options\n");

			char *trail =
			"GGW.... SPL.... HCA.... MCG.... DST.V.." 
			"GDU.... SLO.... HLS.... MTS....";

		Message messages[9] = {};
		DraculaView dv = DvNew(trail, messages);
		
		int numMoves = -1;
		PlaceId *moves = DvGetValidMoves(dv, &numMoves);
		assert(numMoves == 10);
		sortPlaces(moves, numMoves);
		assert(moves[0] == BRUSSELS);
		assert(moves[1] == COLOGNE);
		assert(moves[2] == FRANKFURT);
		assert(moves[3] == GENEVA);
		assert(moves[4] == MUNICH);
		assert(moves[5] == NUREMBURG);
		assert(moves[6] == PARIS);
		assert(moves[7] == ZURICH);
		assert(moves[8] == HIDE);
		assert(moves[9] == DOUBLE_BACK_1);
		free(moves);
		
		printf("Test passed!\n");
		DvFree(dv);
	}

	{///////////////////////////////////////////////////////////////////
	
		printf("DvGetValidMoves Test #3: Double_Back falling off\n");

			char *trail =
			"GST.... SST.... HST.... MST.... DGO.V.. "
			"GST.... SST.... HST.... MST.... DMIT... "
			"GST.... SST.... HST.... MST.... DVET... "
			"GST.... SST.... HST.... MST.... DFLT... "
			"GST.... SST.... HST.... MST....";

		Message messages[24] = {};
		DraculaView dv = DvNew(trail, messages);
		
		int numMoves = -1;
		PlaceId *moves = DvGetValidMoves(dv, &numMoves);
		assert(numMoves == 5);
		sortPlaces(moves, numMoves);
		assert(moves[0] == ROME);
		assert(moves[1] == HIDE);
		assert(moves[2] == DOUBLE_BACK_1);
		assert(moves[3] == DOUBLE_BACK_2);
		assert(moves[4] == DOUBLE_BACK_4);
		free(moves);
		
		printf("Test passed!\n");
		DvFree(dv);
	}

	{///////////////////////////////////////////////////////////////////
	
		printf("DvWhereCanIGo Test #1: General\n");
		
		char *trail =
			"GGE.... SGE.... HGE.... MGE.... DCD.V.. "
			"GGE.... SGE.... HGE.... MGE....";
		
		Message messages[9] = {};
		DraculaView dv = DvNew(trail, messages);
		
		int numMoves = -1;
		PlaceId *moves = DvWhereCanIGo(dv, &numMoves);
		assert(numMoves == 2);
		sortPlaces(moves, numMoves);
		assert(moves[0] == GALATZ);
		assert(moves[1] == KLAUSENBURG);
		free(moves);
		
		printf("Test passed!\n");
		DvFree(dv);
	}

	{///////////////////////////////////////////////////////////////////
	
		printf("DvWhereCanIGo Test #2: Same Location, DB1 & HIDE used\n");
		
		char *trail =
			"GGE.... SGE.... HGE.... MGE.... DKL.V.. "
			"GGE.... SGE.... HGE.... MGE.... DD1T... "
			"GGE.... SGE.... HGE.... MGE.... DBCT... "
			"GGE.... SGE.... HGE.... MGE.... DHIT... "
			"GGE.... SGE.... HGE.... MGE....";
		
		Message messages[24] = {};
		DraculaView dv = DvNew(trail, messages);
		
		int numLocs = -1;
		 PlaceId *locs = DvWhereCanIGo(dv, &numLocs);
		assert(numLocs == 4);
		sortPlaces(locs, numLocs);
		assert(locs[0] == BELGRADE);
		assert(locs[1] == CONSTANTA);
		assert(locs[2] == GALATZ);
		assert(locs[3] == SOFIA);
		free(locs);
		
		printf("Test passed!\n");
		DvFree(dv);
	}
	
	{///////////////////////////////////////////////////////////////////
        
        printf("DvWhereCanTheyGo Test #1: No move made\n");
        
        char *trail = "";
        
        Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
        
        int numLocs = -1;
 
        PlaceId *locs = DvWhereCanTheyGo(dv, PLAYER_LORD_GODALMING, &numLocs);
        assert(numLocs == 0);
        assert(locs == NULL);      
        DvFree(dv);
        printf("Test passed!\n");
    
    }
    
    {///////////////////////////////////////////////////////////////////
        printf("DvWhereCanTheyGo Test #2: [Player] hasn't made a move\n");
        
        char *trail = "";
        
        Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
        
        int numLocs = -1;
        
        PlaceId *locs = DvWhereCanIGo(dv, &numLocs);
        assert(numLocs == 0);
        assert(locs == NULL);
        DvFree(dv);
        printf("Test passed\n");
        
    }
    
    {///////////////////////////////////////////////////////////////////
        printf("DvWhereCanIGo Test #3: First move yet to be\n");
        
        char *trail = "GSZ.... SGE.... HGE....";
        
        Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
        
        int numLocs = -1;
        
        PlaceId *loc = DvWhereCanIGo(dv, &numLocs);
        
        assert(loc == NULL);
        assert(numLocs == 0);
        
        DvFree(dv);
        printf("Test passed\n");
    }

    {///////////////////////////////////////////////////////////////////
        printf("DvWhereCanIGoByType Test #2: [Player] hasn't made a move\n");
        
        char *trail = "";
        
        Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
        
        int numLocs = -1;
        
        PlaceId *locs = DvWhereCanIGoByType(dv, false, true, &numLocs);
        assert(numLocs == 0);
        assert(locs == NULL);
        DvFree(dv);
        printf("Test passed\n");
        
    }
    
    {///////////////////////////////////////////////////////////////////
        printf("DvWhereCanTheyGo Test #2: [Player] hasn't made a move\n");
        
        char *trail = "";
        
        Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
        
        int numLocs = -1;
        
        PlaceId *locs = DvWhereCanTheyGoByType(dv, PLAYER_DR_SEWARD,
                                               true, false, false,  &numLocs);
        assert(numLocs == 0);
        assert(locs == NULL);
        DvFree(dv);
        printf("Test passed\n");
        
    }

	return EXIT_SUCCESS;
}
