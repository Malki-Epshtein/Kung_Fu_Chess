#include "NetworkMessageHandler.h"
#include "../../shared/protocol/Message.h"
#include "../../shared/protocol/MessageCodec.h"
#include "../../shared/protocol/GameSnapshotCodec.h"
#include "../../shared/protocol/CaptureEventCodec.h"
#include "../audio/SoundNames.h"
#include "../../shared/log/Log.h"

namespace {
    // "White"/"Black" -> the other color's display name, for the
    // disconnect-countdown message (Stage D).
    const char* opponentName(const std::string& color) {
        return color == "White" ? "Black" : "White";
    }
}

void NetworkMessageHandler::seedMoveLog(std::vector<MoveEntry> white, std::vector<MoveEntry> black) {
    state_.whiteMoves = std::move(white);//אם שחקן מצטרף באמצי
    state_.blackMoves = std::move(black);
}

NetworkState NetworkMessageHandler::currentState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

void NetworkMessageHandler::handleMoveLogged(const nlohmann::json& payload) {
    MoveLogEvent event = MoveLogCodec::decode(payload);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (event.color == Chess::Color::White)
            state_.whiteMoves.push_back(event.entry);
        else if (event.color == Chess::Color::Black)
            state_.blackMoves.push_back(event.entry);
    }
    soundPlayer_.play(Sounds::MOVE);
}

void NetworkMessageHandler::handleCaptureEvent(const nlohmann::json& payload) {
    CaptureEvent event = CaptureEventCodec::decode(payload);
    spdlog::info("capture event received: kind={} color={} cell={} capturingPieceId={} impactProgress={}",
                  static_cast<int>(event.kind), static_cast<int>(event.color), event.cell,
                  event.capturingPieceId, event.impactProgress);
    // Doesn't play the sound here - queued for checkPendingCaptureSounds()
    // (called once per rendered frame) to fire once the capturing piece's
    // own live travelProgress actually reaches impactProgress, so the
    // sound lines up with what's on screen instead of the raw moment this
    // message arrived.
    std::lock_guard<std::mutex> lock(mutex_);
    pendingCaptures_.push_back(event);
}

// The direct reply to a MOVE/JUMP we just sent - rejected (e.g. illegal,
// not your piece, motion already in progress) is the only case worth a
// sound; a successful one is already covered by MOVE_LOGGED/CAPTURE_EVENT.
void NetworkMessageHandler::handleCommandAck(const nlohmann::json& j) {
    if (!j.value("success", true))
        soundPlayer_.play(Sounds::ILLEGAL_MOVE);
}

void NetworkMessageHandler::handleSnapshot(const nlohmann::json& j) {
    GameSnapshot decoded = GameSnapshotCodec::decode(j);

    // Stage D: the "disconnect" key rides along on the same broadcast
    // rather than being a separate message shape - present only while a
    // seated player's grace-period countdown is active.
    bool active = false;
    std::string message;
    if (j.contains("disconnect") && j["disconnect"].value("active", false)) {
        std::string color = j["disconnect"].value("color", "");
        int secondsRemaining = j["disconnect"].value("secondsRemaining", 0);
        active = true;
        message = secondsRemaining > 0
            ? color + " disconnected - auto-resign in " + std::to_string(secondsRemaining) + "s"
            : color + " resigned (disconnected) - " + opponentName(color) + " wins!";
    }

    // Stage I: score rides along the same way disconnect does.
    int whiteScore = j.value("score", nlohmann::json::object()).value("white", 0);
    int blackScore = j.value("score", nlohmann::json::object()).value("black", 0);

    // Step 4: identity (name/elo/spectatorCount) rides the same way -
    // refreshed every tick, not just once at room-join time.
    nlohmann::json identity = j.value("identity", nlohmann::json::object());
    std::string whiteName    = identity.value("whiteName", std::string());
    int         whiteElo     = identity.value("whiteElo", 0);
    std::string blackName    = identity.value("blackName", std::string());
    int         blackElo     = identity.value("blackElo", 0);
    int         spectatorCount = identity.value("spectatorCount", 0);

    std::lock_guard<std::mutex> lock(mutex_);
    state_.snapshot          = std::move(decoded);
    state_.disconnectActive  = active;
    state_.disconnectMessage = std::move(message);
    state_.whiteScore        = whiteScore;
    state_.blackScore        = blackScore;
    state_.whiteName         = std::move(whiteName);
    state_.whiteElo          = whiteElo;
    state_.blackName         = std::move(blackName);
    state_.blackElo          = blackElo;
    state_.spectatorCount    = spectatorCount;
}

void NetworkMessageHandler::onMessage(const std::string& text) {
    // Runs on WsClient's network thread. Every message the server sends now
    // carries an explicit "type" (see MessageDispatcher::process and
    // GameSession::tick server-side) - MessageCodec::typeFromName is the
    // single source of truth for turning that into a MessageType, so no
    // raw string comparison happens here or anywhere downstream.
    try {
        nlohmann::json j = nlohmann::json::parse(text);
        // The one chokepoint every incoming message passes through
        // regardless of type (snapshot, move log, command ack, ...) -
        // traces full inbound traffic without needing a log line in every
        // individual handleX below.
        spdlog::debug("received: {}", text);
        MessageType type = MessageCodec::typeFromName(j.value("type", std::string()));

        switch (type) {
            case MessageType::MoveLogged:   handleMoveLogged(j.at("payload")); break;
            case MessageType::CaptureEvent: handleCaptureEvent(j.at("payload")); break;
            case MessageType::Snapshot:     handleSnapshot(j); break;
            case MessageType::Move:
            case MessageType::Jump:         handleCommandAck(j); break;
            // Every other type either never reaches this handler (Login/
            // CreateRoom/JoinRoom/FindGame are consumed by their own
            // temporary handlers before this one is even installed - see
            // GraphicalApplication::run()) or carries nothing this screen
            // needs (GameFound, Hello).
            default: break;
        }
    } catch (const std::exception& e) {
        spdlog::error("failed to parse server message: {}", e.what());
    }
}

void NetworkMessageHandler::checkPendingCaptureSounds() {
    // Resolve which pending captures are ready under the lock (touches
    // state_/pendingCaptures_), but actually play sounds after releasing
    // it - same reasoning as handleMoveLogged: soundPlayer_.play() runs a
    // synchronous MCI call, no reason to hold the lock for that.
    int readyCount = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = pendingCaptures_.begin(); it != pendingCaptures_.end(); ) {
            const SnapshotPiece* watched = nullptr;
            for (const auto& p : state_.snapshot.pieces) {
                if (p.id == it->capturingPieceId) {
                    watched = &p;
                    break;
                }
            }

            // See isCaptureSoundReady's own comment for why "not found" and
            // "no longer Moving" both count as ready, not just reaching
            // travelProgress >= impactProgress directly.
            if (isCaptureSoundReady(watched, it->impactProgress)) {
                ++readyCount;
                it = pendingCaptures_.erase(it);
            } else {
                ++it;
            }
        }
    }

    for (int i = 0; i < readyCount; ++i)
        soundPlayer_.play(Sounds::CAPTURE);
}
