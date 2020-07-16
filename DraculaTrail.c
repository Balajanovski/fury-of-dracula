//
// Created by james on 16/7/20.
//

#include <stdbool.h>

#include "Places.h"
#include "DraculaMove.h"

typedef struct {
    PlaceId location;
    bool is_trap;
    bool is_vampire;
} DraculaTrailNode;

typedef DraculaMove* TrailNode;

struct draculaTrail {
    TrailNode trail_head;
    int size;
};