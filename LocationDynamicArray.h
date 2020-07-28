//
// Created by James Balajan on 19/7/20.
//

#ifndef FURY_OF_DRACULA_LOCATIONDYNAMICARRAY_H
#define FURY_OF_DRACULA_LOCATIONDYNAMICARRAY_H

#include "Places.h"

typedef struct locationDynamicArray* LocationDynamicArray;

LocationDynamicArray new_location_dynamic_array();
LocationDynamicArray new_location_dynamic_array_with_capacity(int capacity);

void free_location_dynamic_array(LocationDynamicArray lda);

PlaceId ith_location_location_dynamic_array(LocationDynamicArray lda, int i);
PlaceId ith_latest_location_location_dynamic_array(LocationDynamicArray lda, int i);

void push_back_location_dynamic_array(LocationDynamicArray lda, PlaceId location);

int get_size_location_dynamic_array(LocationDynamicArray lda);
PlaceId* get_raw_array_from_index_location_dynamic_array(LocationDynamicArray lda, int i, int* raw_size);
PlaceId* copy_to_raw_array_from_index_location_dynamic_array(LocationDynamicArray lda, int i, int* copy_size);

void extend_location_dynamic_array(LocationDynamicArray lhs, LocationDynamicArray rhs);
void extend_location_dynamic_array_raw(LocationDynamicArray lhs, PlaceId* rhs, int size);

LocationDynamicArray make_copy_location_dynamic_array(LocationDynamicArray lda);

#endif //FURY_OF_DRACULA_LOCATIONDYNAMICARRAY_H
