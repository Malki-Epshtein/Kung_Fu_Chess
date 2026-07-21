#pragma once
#include "Renderer.h"
#include "src/img.hpp"
#include "ViewConfig.h"
#include "assets/ISpriteSource.h"
#include "../../shared/protocol/MoveEntry.h"
#include <string>
#include <vector>

class ImageView : public Renderer {
private:
    Img background;
    bool backgroundLoaded = false;
    Img canvas;
    bool canvasLoaded = false;
    std::string             assetsRoot;
    ISpriteSource&          spriteSource;
    int                     whiteScore = 0;
    int                     blackScore = 0;
    std::vector<MoveEntry>  whiteMoves;
    std::vector<MoveEntry>  blackMoves;
    std::string             whitePlayerName;
    std::string             blackPlayerName;
    bool                    disconnectActive = false;
    std::string             disconnectMessage;
    std::string             roomName;
    std::string             titledRoomName; // last roomName the OS window title was set to
    int                     spectatorCount = 0;

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
    // fine with neither set - scores start 0-0, move lists start empty)
    // and Renderer's fixed interface takes nothing extra, so they're wired
    // in after construction instead. Plain data, not observer pointers -
    // ImageView runs client-side, where a real ScoreObserver/MoveLogObserver
    // (server-only classes) can never exist; GraphicalApplication decodes
    // this from the wire (GameSnapshot's "score" field, MOVE_LOGGED pushes)
    // and passes it straight through.
    void setScore(int white, int black) {
        whiteScore = white;
        blackScore = black;
    }
    void setMoveLog(std::vector<MoveEntry> white, std::vector<MoveEntry> black) {
        whiteMoves = std::move(white);
        blackMoves = std::move(black);
    }
    void setPlayerNames(std::string white, std::string black) {
        whitePlayerName = std::move(white);//אני לא צריכה יותר את המשאבים של WHITE
        blackPlayerName = std::move(black);
    }
    // Stage D: a disconnect grace-period countdown to show on screen -
    // active=false (the default) draws nothing extra.
    void setDisconnectStatus(bool active, std::string message) {
        disconnectActive = active;
        disconnectMessage = std::move(message);
    }
    // Stage G2c: the room this client just created/joined - shown so
    // everyone in the room can confirm they're in the right one. Empty
    // (the default) draws nothing extra.
    void setRoomName(std::string name) { roomName = std::move(name); }

    // A count, not a name list - matches how real chess sites (lichess,
    // chess.com) show spectators, to avoid a growing/distracting list.
    void setSpectatorCount(int count) { spectatorCount = count; }

    void render(const GameSnapshot& snapshot) override;
};
