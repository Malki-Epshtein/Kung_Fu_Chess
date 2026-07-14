#pragma once
#include "Renderer.h"
#include "src/img.hpp"
#include <string>
#include <unordered_map>

class ImageView : public Renderer {
private:
    Img background;
    bool backgroundLoaded = false;
    std::unordered_map<std::string, Img> spriteCache;

    Img& getSprite(const std::string& pieceFolder, const std::string& stateFolder);

public:
    static constexpr const char* WINDOW_NAME = "Kung Fu Chess";

    void render(const GameSnapshot& snapshot) override;
};
