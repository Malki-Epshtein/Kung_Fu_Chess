#pragma once
#include "../model/Position.h"

struct Motion {
    Position from;
    Position to;
    int      arrival_time_ms;
    int      piece_id;
};
