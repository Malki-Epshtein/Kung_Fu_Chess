#pragma once
#include "Renderer.h"

class ImageView : public Renderer {
public:
    void render(const GameSnapshot& snapshot) override;
};
