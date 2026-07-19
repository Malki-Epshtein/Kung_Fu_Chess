#pragma once
#include <string>
#include "../../../shared/model/Piece.h"

struct AnimationConfig {
    double       speed_m_per_sec;
    Chess::State next_state_when_finished;
    int          frames_per_sec;
    int          frame_count;
    bool         is_loop;
};

AnimationConfig loadAnimationConfig(const std::string& path);
