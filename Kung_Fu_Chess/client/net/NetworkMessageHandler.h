#pragma once
#include "../audio/SoundPlayer.h"
#include "../../shared/engine/GameSnapshot.h"
#include "../../shared/protocol/MoveLogCodec.h"
#include "../../shared/protocol/CaptureEvent.h"
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

    // Animation-event-style capture-sound timing (see CaptureEvent's own
    // comment) - called once per rendered frame from the render loop
    // (GraphicalApplication), NOT from onMessage. A CAPTURE_EVENT arriving
    // (handleCaptureEvent) only queues it; this is what actually decides
    // when to play the sound, by checking each pending capture's watched
    // piece against the *current* snapshot's live travelProgress - the
    // exact same interpolation value already driving where that piece is
    // drawn this frame. Fires immediately (no further waiting) for a
    // pending capture whose watched piece is no longer in the snapshot at
    // all, so a same-tick capture-and-removal (see ArrivalResolver) can
    // never stall silently forever.
    void checkPendingCaptureSounds();

    // The pure decision behind checkPendingCaptureSounds(): has the watched
    // piece's own live animation state reached the recorded impact point
    // yet? Pulled out as a standalone, header-inline predicate (no
    // SoundPlayer/mutex involved) so it's directly unit-testable without
    // needing NetworkMessageHandler.cpp/SoundPlayer.cpp in the test build.
    // watched == nullptr (piece no longer in the snapshot at all) and
    // watched->state != Moving (its motion already finished and was
    // dropped from active_motions - see RealTimeArbiter::tick - before
    // travelProgress could ever show it "finished") both count as ready,
    // since travelProgress alone would otherwise never reach a normal
    // arrival capture's impactProgress==1.0 for a piece no longer moving.
    static bool isCaptureSoundReady(const SnapshotPiece* watched, double impactProgress) {
        return !watched || watched->state != Chess::State::Moving
            || watched->travelProgress >= impactProgress;
    }

private:
    void handleMoveLogged(const nlohmann::json& payload);
    void handleCaptureEvent(const nlohmann::json& payload);
    void handleCommandAck(const nlohmann::json& j);
    void handleSnapshot(const nlohmann::json& j);

    SoundPlayer&              soundPlayer_;
    mutable std::mutex        mutex_;
    NetworkState              state_;
    std::vector<CaptureEvent> pendingCaptures_;
};
