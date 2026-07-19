#pragma once
#include "../../shared/engine/GameSnapshot.h"

class Renderer {
public:
    virtual void render(const GameSnapshot& snapshot) = 0;
    virtual ~Renderer() = default;
};
