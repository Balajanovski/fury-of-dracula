//
// Created by James Balajan on 19/7/20.
//

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "Places.h"
#include "LocationDynamicArray.h"

#define DEFAULT_CAPACITY 10


struct locationDynamicArray {
    PlaceId* past_locations;
    int history_size;
    int history_capacity;
};


LocationDynamicArray new_location_dynamic_array() {
    LocationDynamicArray new_lh = (LocationDynamicArray) malloc(sizeof(struct locationDynamicArray));
    new_lh->history_capacity = DEFAULT_CAPACITY;
    new_lh->history_size = 0;
    new_lh->past_locations = (PlaceId*) malloc(sizeof(PlaceId) * DEFAULT_CAPACITY);

    return new_lh;
}

void free_location_dynamic_array(LocationDynamicArray lda) {
    assert(lda != NULL);
    assert(lda->past_locations != NULL);

    free(lda->past_locations);
    free(lda);
}

PlaceId ith_location_location_dynamic_array(LocationDynamicArray lda, int i) {
    if (i < 0 || i >= get_size_location_dynamic_array(lda)) {
        fprintf(stderr, "Out of bounds access of ith location (i-th: %d, size: %d)\n", i, lda->history_size);
        exit(1);
    }

    return lda->past_locations[i];
}

PlaceId ith_latest_location_location_dynamic_array(LocationDynamicArray lda, int i) {
    int index = lda->history_size - i - 1;
    return ith_location_location_dynamic_array(lda, index);
}

void push_back_location_dynamic_array(LocationDynamicArray lda, PlaceId location) {
    if (lda->history_size >= lda->history_capacity) {
        lda->history_capacity *= 2;

        PlaceId* realloced_past_locations = (PlaceId*) realloc(lda->past_locations, sizeof(PlaceId) * lda->history_capacity);
        if (realloced_past_locations == NULL) {
            fprintf(stderr, "Unable to realloc in LocationDynamicArray.c. Aborting...\n");
            exit(EXIT_FAILURE);
        } else {
            lda->past_locations = realloced_past_locations;
        }
    }

    lda->past_locations[lda->history_size++] = location;
}

int get_size_location_dynamic_array(LocationDynamicArray lda) {
    return lda->history_size;
}

PlaceId* get_raw_array_from_index_location_dynamic_array(LocationDynamicArray lda, int i, int* raw_size) {
    int lda_size = get_size_location_dynamic_array(lda);
    if (i >= lda_size || i < 0) {
        fprintf(stderr, "Out of bounds get raw array. Aborting...\n");
        exit(1);
    }
    *raw_size = lda_size - i;

    return lda->past_locations + i;
}

PlaceId* copy_to_raw_array_from_index_location_dynamic_array(LocationDynamicArray lda, int i, int* copy_size) {
    int lda_size = get_size_location_dynamic_array(lda);
    if (i >= lda_size || i < 0) {
        fprintf(stderr, "Out of bounds copy to raw array. Aborting...\n");
        exit(1);
    }

    int copied_array_size = *copy_size = lda_size - i;
    PlaceId* copy = (PlaceId*) malloc(sizeof(PlaceId) * copied_array_size);

    int insertion_index = 0;
    for (; i < lda_size; ++i, ++insertion_index) {
        copy[insertion_index] = ith_location_location_dynamic_array(lda, i);
    }

    return copy;
}

void extend_location_dynamic_array(LocationDynamicArray lhs, LocationDynamicArray rhs) {
    if (lhs == NULL) {
        fprintf(stderr, "Attempted to extend NULL lhs dynamic array. Aborting...\n");
        exit(1);
    }

    if (rhs == NULL) {
        return;
    }

    int rhs_size = get_size_location_dynamic_array(rhs);
    for (int i = 0; i < rhs_size; ++i) {
        push_back_location_dynamic_array(lhs, ith_location_location_dynamic_array(rhs, i));
    }
}

void extend_location_dynamic_array_raw(LocationDynamicArray lhs, PlaceId* rhs, int size) {
    if (lhs == NULL) {
        fprintf(stderr, "Attempted to extend NULL lhs dynamic array. Aborting...\n");
        exit(1);
    }

    if (rhs == NULL) {
        return;
    }

    for (int i = 0; i < size; ++i) {
        push_back_location_dynamic_array(lhs, rhs[i]);
    }
}
