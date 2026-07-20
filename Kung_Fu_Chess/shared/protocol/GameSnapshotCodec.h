#pragma once
#include "../engine/GameSnapshot.h"
#include "json.hpp"

class GameSnapshotCodec {
public:
    static nlohmann::json encode(const GameSnapshot& snapshot);
    static GameSnapshot   decode(const nlohmann::json& j);
};
