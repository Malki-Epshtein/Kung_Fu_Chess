#pragma once
#include "Renderer.h"
#include "src/img.hpp"
#include "AnimationConfig.h"
#include <string>
#include <unordered_map>

class ImageView : public Renderer {
private:
    Img background;
    bool backgroundLoaded = false;
    std::unordered_map<std::string, Img> spriteCache;
    std::unordered_map<std::string, AnimationConfig> configCache;
    std::unordered_map<std::string, int> frameCountCache;

    Img& getSprite(const std::string& pieceFolder, const std::string& stateFolder, int frameIndex);
    const AnimationConfig& getConfig(const std::string& pieceFolder, const std::string& stateFolder);
    int getFrameCount(const std::string& pieceFolder, const std::string& stateFolder);

public:
    static constexpr const char* WINDOW_NAME = "Kung Fu Chess";

    void render(const GameSnapshot& snapshot) override;
};
