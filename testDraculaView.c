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
	
		printf("DvGetValidMoves Test #1: No moves yet \n");
		
		char *trail =
			"GGE.... SGE.... HGE....";
		
		Message messages[9] = {};
		DraculaView dv = DvNew(trail, messages);
		
		int numMoves = -1;
		PlaceId *moves = DvGetValidMoves(dv, &numMoves);
		assert(numMoves == 0);
		assert(moves == NULL);
		free(moves);
		
		printf("Test passed!\n");
		DvFree(dv);
	}
	
	{///////////////////////////////////////////////////////////////////
	
		printf("DvGetValidMoves Test #2: Many options\n");

		char *trail =
			"GGW.... SPL.... HCA.... MCG.... DST.V.." 
			"GDU.... SLO.... HLS.... MTS....";

		Message messages[5] = {};
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
	
		printf("DvGetValidMoves Test #3: Quintuple Double backs\n");

		char *trail =
			"GZA.... SZA.... HZA.... MZA.... DFR.V.."
			"GZA.... SZA.... HZA.... MZA.... DNUT..."
			"GZA.... SZA.... HZA.... MZA.... DMUT..."
			"GZA.... SZA.... HZA.... MZA.... DZUT..."
			"GZA.... SZA.... HZA.... MZA.... DGET..."
			"GZA.... SZA.... HZA.... MZA.... DSTT..."
			"GZA.... SZA.... HZA.... MZA....";

		Message messages[5] = {};
		DraculaView dv = DvNew(trail, messages);
		
		int numMoves = -1;
		PlaceId *moves = DvGetValidMoves(dv, &numMoves);
		assert(numMoves == 10);
		sortPlaces(moves, numMoves);
		assert(moves[0] == BRUSSELS);
		assert(moves[1] == COLOGNE);
		assert(moves[2] == FRANKFURT);
		assert(moves[3] == PARIS);
		assert(moves[4] == HIDE);
		assert(moves[5] == DOUBLE_BACK_1);
		assert(moves[6] == DOUBLE_BACK_2);
		assert(moves[7] == DOUBLE_BACK_3);
		assert(moves[8] == DOUBLE_BACK_4);
		assert(moves[9] == DOUBLE_BACK_5);
		free(moves);
		
		printf("Test passed!\n");
		DvFree(dv);
	}
	
	{///////////////////////////////////////////////////////////////////

		printf("DvGetValidMoves Test #4: Able to move to start location\n");
		printf("Process: Sixth slot in trail, a move becomes valid once more");
		char *trail =
			"GST.... SST.... HST.... MST.... DMA.V.."
			"GST.... SST.... HST.... MST.... DD1T..." 
			"GST.... SST.... HST.... MST.... DLST..." 
			"GST.... SST.... HST.... MST.... DCAT..."
			"GST.... SST.... HST.... MST.... DGRT..."
			"GST.... SST.... HST.... MST.... DALT..."
			"GST.... SST.... HST.... MST...."; 

		Message messages[5] = {};
		DraculaView dv = DvNew(trail, messages);
		
		int numMoves = -1;
		PlaceId *moves = DvGetValidMoves(dv, &numMoves);
		assert(numMoves == 4);
		sortPlaces(moves, numMoves);
		assert(moves[0] == MADRID);
		assert(moves[1] == MEDITERRANEAN_SEA);
		assert(moves[2] == SARAGOSSA);
		assert(moves[3] == HIDE);
		free(moves);
		
		printf("Test passed!\n");
		DvFree(dv);
	}

	{///////////////////////////////////////////////////////////////////

		printf("DvGetValidMoves Test #5: DOUBLE_BACK as 6th prev. round\n");
		char *trail =
			"GST.... SST.... HST.... MST.... DMA.V.."
			"GST.... SST.... HST.... MST.... DD1T..." 
			"GST.... SST.... HST.... MST.... DLST..." 
			"GST.... SST.... HST.... MST.... DCAT..."
			"GST.... SST.... HST.... MST.... DGRT..."
			"GST.... SST.... HST.... MST.... DALT..."
			"GST.... SST.... HST.... MST.... DSRT.V." 
			"GST.... SST.... HST.... MST....";

		Message messages[5] = {};
		DraculaView dv = DvNew(trail, messages);
	
		int numMoves = -1;
		PlaceId *moves = DvGetValidMoves(dv, &numMoves);
		assert(numMoves == 8);
		sortPlaces(moves, numMoves);
		assert(moves[0] == BARCELONA);
		assert(moves[1] == BORDEAUX);
		assert(moves[2] == MADRID);
		assert(moves[3] == SANTANDER);		
		assert(moves[4] == TOULOUSE);
		assert(moves[5] == HIDE);
		assert(moves[6] == DOUBLE_BACK_1);
		assert(moves[7] == DOUBLE_BACK_2);
		
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
        
		printf("DvWhereCanIGo Test #3: Dracula hasn't made a move\n");
        
        char *trail = "";
        
        Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
        
        int numLocs = -1;
        
        PlaceId *locs = DvWhereCanIGo(dv, &numLocs);
        assert(numLocs == 0);
        assert(locs == NULL);
        
		free(locs);
		DvFree(dv);
        printf("Test passed\n");
        
    }

    {///////////////////////////////////////////////////////////////////
        
		printf("DvWhereCanIGo Test #4: First move yet to be\n");
        
        char *trail = "GSZ.... SGE.... HGE....";

        Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
        
        int numLocs = -1;
        
        PlaceId *loc = DvWhereCanIGo(dv, &numLocs);
        
        assert(loc == NULL);
        assert(numLocs == 0);
        
		free(loc);
        DvFree(dv);
        printf("Test passed\n");
    }

	{///////////////////////////////////////////////////////////////////
        
		printf("DvWhereCanIGo Test #4: Double back'd location can be visited\n");
        printf("Process: Madrid is at the end of trail and thus, is valid.\n");
        char *trail = 
			"GST.... SST.... HST.... MST.... DMA.V.."
			"GST.... SST.... HST.... MST.... DD1T..." 
			"GST.... SST.... HST.... MST.... DLST..." 
			"GST.... SST.... HST.... MST.... DCAT..."
			"GST.... SST.... HST.... MST.... DGRT..."
			"GST.... SST.... HST.... MST.... DALT..."
			"GST.... SST.... HST.... MST...."; 
        Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
    
        int numLocs = -1;
        PlaceId *locs = DvWhereCanIGo(dv, &numLocs);
        assert(numLocs == 3);
		sortPlaces(locs, numLocs);
		assert(locs[0] == MADRID);

		free(locs);
        DvFree(dv);
        printf("Test passed\n");
    }

	{///////////////////////////////////////////////////////////////////
		printf("DvWhereCanIGo Test #5: St Joseph? No thank you.\n");
        
		char *trail = 
			"GRO.... SMA.... HMN.... MNU.... DSZ.V..";			

		Message messages[5] = {};
		DraculaView dv = DvNew(trail, messages);

		int numLocs = -1;
		PlaceId *locs = DvWhereCanIGo(dv, &numLocs);
		sortPlaces(locs, numLocs);
		assert(numLocs == 4);
		assert(locs[0] == BELGRADE);
		assert(locs[1] == BUDAPEST);
		assert(locs[2] == KLAUSENBURG);
		assert(locs[3] != ST_JOSEPH_AND_ST_MARY);
		assert(locs[3] == ZAGREB);

		free(locs);
		DvFree(dv);
		printf("Test passed\n");
	}
	{///////////////////////////////////////////////////////////////////
        
        printf("DvWhereCanTheyGo Test #1: No moves made by any players\n");
        
        char *trail = "";
        
        Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
        
        int numLocs = -1;
 
        PlaceId *locs = DvWhereCanTheyGo(dv, PLAYER_LORD_GODALMING, &numLocs);
        assert(numLocs == 0);
        assert(locs == NULL);      
		DvFree(dv);
		

		dv = DvNew(trail, messages);
        locs = DvWhereCanTheyGo(dv, PLAYER_DR_SEWARD, &numLocs);
        assert(numLocs == 0);
        assert(locs == NULL);
		DvFree(dv);
		

		dv = DvNew(trail, messages);
		locs = DvWhereCanTheyGo(dv, PLAYER_MINA_HARKER, &numLocs);
        assert(numLocs == 0);
        assert(locs == NULL);
	    DvFree(dv);

		dv = DvNew(trail, messages);
		locs = DvWhereCanTheyGo(dv, PLAYER_VAN_HELSING, &numLocs);
        assert(numLocs == 0);
        assert(locs == NULL);
        DvFree(dv);
		
		dv = DvNew(trail, messages);
		locs = DvWhereCanTheyGo(dv, PLAYER_DRACULA, &numLocs);
        assert(numLocs == 0);
        assert(locs == NULL);
        DvFree(dv);

        printf("Test passed!\n");
    
    }

    {///////////////////////////////////////////////////////////////////
		printf("DvWhereCanTheyGo Test #2: Hunters' move\n");

		char *trail = 
			"GRO.... SLS.... HMN.... MNU....";			

		Message messages[5] = {};
		DraculaView dv = DvNew(trail, messages);

		int numLocs = -1;

		PlaceId *locs = DvWhereCanTheyGo(dv, PLAYER_LORD_GODALMING, &numLocs);
		assert(numLocs == 5);
		sortPlaces(locs, numLocs);
		assert(locs[0] == BARI);
		assert(locs[1] == FLORENCE);
		assert(locs[2] == NAPLES);
		assert(locs[3] == ROME);
		assert(locs[4] == TYRRHENIAN_SEA);
		free(locs);

		locs = DvWhereCanTheyGo(dv, PLAYER_DR_SEWARD, &numLocs);
		assert(numLocs == 7);
		sortPlaces(locs, numLocs);
		assert(locs[0] == ALICANTE);
		assert(locs[1] == ATLANTIC_OCEAN);
		assert(locs[2] == CADIZ);
		assert(locs[3] == LISBON);
		assert(locs[4] == MADRID);
		assert(locs[5] == SANTANDER);
		assert(locs[6] == SARAGOSSA);
		free(locs);
		
		locs = DvWhereCanTheyGo(dv, PLAYER_VAN_HELSING, &numLocs);
		assert(numLocs == 5);
		sortPlaces(locs, numLocs);
		assert(locs[0] == EDINBURGH);
		assert(locs[1] == LIVERPOOL);
		assert(locs[2] == LONDON);
		assert(locs[3] == MANCHESTER);
		assert(locs[4] == SWANSEA);
		free(locs);

		locs = DvWhereCanTheyGo(dv, PLAYER_MINA_HARKER, &numLocs);
		assert(numLocs == 6);
		sortPlaces(locs, numLocs);
		assert(locs[0] == FRANKFURT);
		assert(locs[1] == LEIPZIG);
		assert(locs[2] == MUNICH);
		assert(locs[3] == NUREMBURG);
		assert(locs[4] == PRAGUE);
		assert(locs[5] == STRASBOURG);
		free(locs);
	
		DvFree(dv);
        printf("Test passed!\n");
	}

    {///////////////////////////////////////////////////////////////////
        
		printf("DvWhereCanTheyGoByType Test #1: Hunter has no options, everything false\n");
        
        char *trail = "GSZ.... SGE.... HGE....";
        
        Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
        
        int numLocs = -1;
        
        PlaceId *locs = DvWhereCanTheyGoByType(dv, PLAYER_MINA_HARKER, 
                        false, false, false, &numLocs);
                        
        assert(numLocs == 0);
        assert(locs == NULL);
        
        DvFree(dv);
        printf("Test passed\n");
    
    }
    
    {///////////////////////////////////////////////////////////////////
        
		printf("DvWhereCanIGoByType Test #1: Road connections \n");
        
        char *trail1 = "GGE.... SGE.... HGE.... MGE.... DKL.V..";
        
        Message messages[5] = {};
        DraculaView dv = DvNew(trail1, messages);
        
        int numLocs1 = -1;
		int numLocs2 = -1;
        
        PlaceId *locs = DvWhereCanIGoByType(dv, true, true, &numLocs1);
        
        assert(numLocs1 == 6);
        sortPlaces(locs, numLocs1);
		assert(locs[0] == BELGRADE);
		assert(locs[1] == BUCHAREST);
		assert(locs[2] == BUDAPEST);
		assert(locs[3] == CASTLE_DRACULA);
		assert(locs[4] == GALATZ);
		assert(locs[5] == SZEGED);
		free(locs);

		PlaceId *locsFalse = DvWhereCanIGoByType(dv, false, false, &numLocs2);
        assert(numLocs2 == 0);
        assert(locsFalse == NULL);
        free(locsFalse);

		DvFree(dv);
		printf("Test passed\n");

	}

    {///////////////////////////////////////////////////////////////////
        
		printf("DvWhereCanIGoByType Test #2: Boat connections \n");

		char *trail2 = "GGE.... SGE.... HGE.... MGE.... DMS....";
		
		Message messages[5] = {};
		DraculaView dv = DvNew(trail2, messages);

		int numLocs = -1;

		PlaceId *locsBoat = DvWhereCanIGoByType(dv, true, true, &numLocs);
		assert(numLocs == 6);
		sortPlaces(locsBoat, numLocs);
		assert(locsBoat[0] == ALICANTE);
		assert(locsBoat[1] == ATLANTIC_OCEAN);
		assert(locsBoat[2] == BARCELONA);
		assert(locsBoat[3] == CAGLIARI);
		assert(locsBoat[4] == MARSEILLES);
		assert(locsBoat[5] == TYRRHENIAN_SEA);
		free(locsBoat);
        
		PlaceId *locsFalse = DvWhereCanIGoByType(dv, true, false, &numLocs);
        assert(numLocs == 0);
        assert(locsFalse == NULL);
        free(locsFalse);
    
		DvFree(dv);
        printf("Test passed\n");

	}

    {///////////////////////////////////////////////////////////////////
		
		printf("DvGetVampireLocation Test #1: Immature vampire remains\n");
		
		char *trail = 
			"GBU.... SGE.... HBR.... MMU.... DPR.V.."
            "GCO.... SGE.... HHA.... MMU.... DNUT..."
			"GFR.... SGE.... HBR.... MMU.... DSTT...";

		Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
		PlaceId vampLoc = DvGetVampireLocation(dv);
		assert(vampLoc == PRAGUE);

		DvFree(dv);
        printf("Test passed\n");
	}

    {///////////////////////////////////////////////////////////////////
		
		printf("DvGetVampireLocation (DvGetHealth) Test #2: Immature vampire is dead and Dracula gets staked\n");
		
		char *trail = 
			"GBU.... SGE.... HBR.... MMU.... DPR.V.."
            "GCO.... SGE.... HPRVD.. MMU.... DNUT...";

		Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
		PlaceId vampLoc = DvGetVampireLocation(dv);
		assert(vampLoc == NOWHERE);
		assert(DvGetHealth(dv, PLAYER_DRACULA) == GAME_START_BLOOD_POINTS - LIFE_LOSS_HUNTER_ENCOUNTER);

		DvFree(dv);
        printf("Test passed\n");
	}

    {///////////////////////////////////////////////////////////////////

		printf("DvGetVampireLocation (DvGetScore) Test #3: Go forth vampire!\n");

		char *trail =
			"GBU.... SGE.... HBR.... MMU.... DPR.V.."
			"GCO.... SGE.... HBR.... MMU.... DNUT..."
			"GFR.... SGE.... HBR.... MMU.... DSTT..."
			"GLI.... SGE.... HBR.... MMU.... DPAT..."
			"GHA.... SGE.... HBR.... MMU.... DLET..."
			"GNS.... SGE.... HBR.... MMU.... DEC...."
			"GED.... SGE.... HBR.... MMU.... DPLT.V."
			"GMN.... SGE.... HBR.... MMU.... DHIT.M.";

		Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
		PlaceId vampLoc = DvGetVampireLocation(dv);
		assert(vampLoc == NOWHERE);
		assert(DvGetScore(dv) == GAME_START_SCORE - (SCORE_LOSS_DRACULA_TURN * 8) - SCORE_LOSS_VAMPIRE_MATURES);

		DvFree(dv);
		printf("Test passed\n");

	}

	{///////////////////////////////////////////////////////////////////

		printf("DvGetScore Test #1: No immature vampire, no score loss\n");
		
		char *trail =
			"GBU.... SGE.... HBR.... MMU.... DPR.V.."
			"GCO.... SGE.... HBR.... MMU.... DNUT..."
			"GFR.... SGE.... HPRV... MMU.... DSTT..."
			"GLI.... SGE.... HVI.... MMU.... DPAT..."
			"GHA.... SGE.... HVI.... MMU.... DLET..."
			"GNS.... SGE.... HVI.... MMU.... DEC...."
			"GED.... SGE.... HVI.... MMU.... DPLT..."
			"GMN.... SGE.... HVI.... MMU.... DHIT.M.";
	
		Message messages[5] = {};
        DraculaView dv = DvNew(trail, messages);
		PlaceId vampLoc = DvGetVampireLocation(dv);
		assert(vampLoc == NOWHERE);
		assert(DvGetScore(dv) == GAME_START_SCORE - (SCORE_LOSS_DRACULA_TURN * 8));

		DvFree(dv);
		printf("Test passed\n");
	}

	return EXIT_SUCCESS;
}
