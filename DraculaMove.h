//
// Created by james on 16/7/20.
//

#ifndef FURY_OF_DRACULA_DRACULAMOVE_H
#define FURY_OF_DRACULA_DRACULAMOVE_H

#include <stdbool.h>

#include "Places.h"


typedef struct {
    bool placed_trap;
    bool placed_vampire;
    PlaceId location;
} DraculaMove;

#endif //FURY_OF_DRACULA_DRACULAMOVE_H
