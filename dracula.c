////////////////////////////////////////////////////////////////////////
// COMP2521 20T2 ... the Fury of Dracula
// dracula.c: your "Fury of Dracula" Dracula AI
//
// 2014-07-01	v1.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2017-12-01	v1.1	Team Dracula <cs2521@cse.unsw.edu.au>
// 2018-12-31	v2.0	Team Dracula <cs2521@cse.unsw.edu.au>
// 2020-07-10	v3.0	Team Dracula <cs2521@cse.unsw.edu.au>
//
////////////////////////////////////////////////////////////////////////

#include "dracula.h"
#include "DraculaView.h"
#include "Game.h"
#include "kTree.h"

/*
typedef struct {
    DraculaView current_view;
} GameState;

static void custom_dracula_view_free(void* value) {
    DvFree((DraculaView) value);
}

static Item create_game_state_item(DraculaView value) {
    Item new_item;
    new_item.data = malloc(sizeof(DraculaView*));
    *((DraculaView*) new_item.data) = value;
    new_item.custom_free = &custom_dracula_view_free;

    return new_item;
}
 */

void decideDraculaMove(DraculaView dv) {
	// TODO: Replace this with something better!
	registerBestPlay("CD", "Mwahahahaha");
}
