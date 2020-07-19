//
// Created by James Balajan on 19/7/20.
//

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "Places.h"
#include "LocationHistory.h"

#define DEFAULT_CAPACITY 10


struct locationHistory {
    PlaceId* past_locations;
    int history_size;
    int history_capacity;
};


LocationHistory new_location_history() {
    LocationHistory new_lh = (LocationHistory) malloc(sizeof(struct locationHistory));
    new_lh->history_capacity = DEFAULT_CAPACITY;
    new_lh->history_size = 0;
    new_lh->past_locations = (PlaceId*) malloc(sizeof(PlaceId) * DEFAULT_CAPACITY);

    return new_lh;
}

void free_location_history(LocationHistory lh) {
    assert(lh != NULL);
    assert(lh->past_locations != NULL);

    free(lh->past_locations);
    free(lh);
}

PlaceId ith_latest_location_location_history(LocationHistory lh, int i) {
    if (lh->history_size - i - 1 < 0) {
        fprintf(stderr, "Out of bounds access of latest ith location (last i-th: %d, size: %d)\n", i, lh->history_size);
        exit(1);
    }

    return lh->past_locations[lh->history_size - i - 1];
}

void push_location_history(LocationHistory lh, PlaceId location) {
    if (lh->history_size >= lh->history_capacity) {
        lh->history_capacity *= 2;
        lh->past_locations = (PlaceId*) realloc(lh->past_locations, sizeof(PlaceId) * lh->history_capacity);
    }

    lh->past_locations[lh->history_size++] = location;
}

int get_size_location_history(LocationHistory lh) {
    return lh->history_size;
}

PlaceId* get_raw_array_from_index_location_history(LocationHistory lh, int i) {
    return lh->past_locations + i;
}
