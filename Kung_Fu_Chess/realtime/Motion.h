#pragma once
#include "../model/Position.h"

struct Motion {
    Position from;
    Position to;
    int      start_time_ms;
    int      arrival_time_ms;
    int      piece_id;
};
