#pragma once
#include "Renderer.h"
#include "src/img.hpp"
#include "ViewConfig.h"
#include "assets/ISpriteSource.h"
#include "../../server/engine/ScoreObserver.h"
#include "../../server/engine/MoveLogObserver.h"
#include <string>

class ImageView : public Renderer {
private:
    Img background;
    bool backgroundLoaded = false;
    Img canvas;
    bool canvasLoaded = false;
    std::string             assetsRoot;
    ISpriteSource&          spriteSource;
    const ScoreObserver*    scoreObserver = nullptr;
    const MoveLogObserver*  moveLogObserver = nullptr;
    std::string             whitePlayerName;
    std::string             blackPlayerName;
    bool                    disconnectActive = false;
    std::string             disconnectMessage;

public:
    // Both constructor-injected: rendering is meaningless without a sprite
    // source or a way to locate the board/canvas backgrounds. ImageView
    // depends on the ISpriteSource abstraction, not on SpriteRepository
    // directly (Dependency Inversion) - so it can be rendered against a
    // fake sprite source in tests, with no disk access for sprites. The
    // resolved assetsRoot is passed in rather than read here, so this
    // class no longer needs to know paths_config.txt exists.
    ImageView(ISpriteSource& spriteSource, std::string assetsRoot)
        : assetsRoot(std::move(assetsRoot)), spriteSource(spriteSource) {}

    // Setters, not constructor params: both are optional (render() works
    // fine with neither set) and Renderer's fixed interface takes no
    // observers, so they're wired in after construction instead.
    void setScoreObserver(const ScoreObserver* observer) { scoreObserver = observer; }
    void setMoveLogObserver(const MoveLogObserver* observer) { moveLogObserver = observer; }
    void setPlayerNames(std::string white, std::string black) {
        whitePlayerName = std::move(white);
        blackPlayerName = std::move(black);
    }
    // Stage D: a disconnect grace-period countdown to show on screen -
    // active=false (the default) draws nothing extra.
    void setDisconnectStatus(bool active, std::string message) {
        disconnectActive = active;
        disconnectMessage = std::move(message);
    }

    void render(const GameSnapshot& snapshot) override;
};
