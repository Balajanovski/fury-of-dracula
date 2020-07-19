//
// Created by James Balajan on 19/7/20.
//

#ifndef FURY_OF_DRACULA_LOCATIONHISTORY_H
#define FURY_OF_DRACULA_LOCATIONHISTORY_H

#include "Places.h"

typedef struct locationHistory* LocationHistory;

LocationHistory new_location_history();
void free_location_history(LocationHistory lh);

PlaceId ith_latest_location_location_history(LocationHistory lh, int i);
void push_location_history(LocationHistory lh, PlaceId location);

int get_size_location_history(LocationHistory lh);
PlaceId* get_raw_array_from_index_location_history(LocationHistory lh, int i);

#endif //FURY_OF_DRACULA_LOCATIONHISTORY_H
