#pragma once
#include "../audio/SoundPlayer.h"
#include "../../shared/engine/GameSnapshot.h"
#include "../../shared/protocol/MoveLogCodec.h"
#include "json.hpp"
#include <mutex>
#include <string>
#include <vector>

// Everything currentState() hands back in one shot - the render loop's own
// per-frame "copy everything out under one lock" snapshot, bundled into a
// struct instead of a dozen separate out-parameters/locals.
struct NetworkState {
    GameSnapshot            snapshot;
    bool                    disconnectActive = false;
    std::string             disconnectMessage;
    int                     whiteScore = 0;
    int                     blackScore = 0;
    std::vector<MoveEntry>  whiteMoves;
    std::vector<MoveEntry>  blackMoves;
    std::string             whiteName;
    int                     whiteElo = 0;
    std::string             blackName;
    int                     blackElo = 0;
    int                     spectatorCount = 0;
};

// Owns everything that arrives from the server once a room is joined:
// parses each inbound message and updates its own state, triggering the
// sounds a network event implies (move/capture/illegal-move) along the way.
// onMessage runs on WsClient's network thread; currentState() is read from
// the GUI thread each frame - mutex-guarded, same as when this state lived
// directly on GraphicalApplication. Split out on 2026-07-22 because message
// parsing/dispatch is a distinct reason to change from the render loop or
// app composition (Single Responsibility) - see
// [[project-kungfu-chess-architecture]] memory for the fuller rationale.
class NetworkMessageHandler {
public:
    explicit NetworkMessageHandler(SoundPlayer& soundPlayer) : soundPlayer_(soundPlayer) {}

    void onMessage(const std::string& text);

    // One-time backfill for a room already in progress - safe to call
    // before onMessage is ever installed (no concurrent access yet).
    void seedMoveLog(std::vector<MoveEntry> white, std::vector<MoveEntry> black);

    NetworkState currentState() const;

private:
    void handleMoveLogged(const nlohmann::json& payload);
    void handleCaptureEvent(const nlohmann::json& payload);
    void handleCommandAck(const nlohmann::json& j);
    void handleSnapshot(const nlohmann::json& j);

    SoundPlayer&        soundPlayer_;
    mutable std::mutex  mutex_;
    NetworkState        state_;
};
