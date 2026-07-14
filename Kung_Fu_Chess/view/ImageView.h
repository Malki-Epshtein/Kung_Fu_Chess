#pragma once
#include "Renderer.h"
#include "src/img.hpp"
#include "AnimationConfig.h"
#include "../engine/ScoreObserver.h"
#include "../engine/MoveLogObserver.h"
#include <string>
#include <unordered_map>

class ImageView : public Renderer {
private:
    Img background;
    bool backgroundLoaded = false;
    Img canvas;
    bool canvasLoaded = false;
    std::unordered_map<std::string, Img> spriteCache;
    std::unordered_map<std::string, AnimationConfig> configCache;
    std::unordered_map<std::string, int> frameCountCache;
    std::string assetsRoot;
    bool assetsRootLoaded = false;
    const ScoreObserver*   scoreObserver = nullptr;
    const MoveLogObserver* moveLogObserver = nullptr;

    const std::string& getAssetsRoot();
    Img& getSprite(const std::string& pieceFolder, const std::string& stateFolder, int frameIndex);
    const AnimationConfig& getConfig(const std::string& pieceFolder, const std::string& stateFolder);
    int getFrameCount(const std::string& pieceFolder, const std::string& stateFolder);

public:
    static constexpr const char* WINDOW_NAME = "Kung Fu Chess";
    // Side-panel layout: board sits centered between a left and right panel
    // of this width, each showing one player's score/last move. Shared with
    // GraphicalApplication so raw mouse-click x can be corrected back to
    // board-local coordinates before reaching BoardMapper/Controller.
    static constexpr int PANEL_WIDTH = 280;
    static constexpr int BOARD_WIDTH_PX = 800;

    // Setters, not constructor params: both are optional (render() works
    // fine with neither set) and Renderer's fixed interface takes no
    // observers, so they're wired in after construction instead.
    void setScoreObserver(const ScoreObserver* observer) { scoreObserver = observer; }
    void setMoveLogObserver(const MoveLogObserver* observer) { moveLogObserver = observer; }

    void render(const GameSnapshot& snapshot) override;
};
