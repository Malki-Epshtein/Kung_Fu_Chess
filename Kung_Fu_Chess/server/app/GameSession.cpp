#include "GameSession.h"
#include "../../shared/protocol/GameSnapshotCodec.h"

namespace {
    const char* colorName(Chess::Color color) {
        switch (color) {
            case Chess::Color::White: return "White";
            case Chess::Color::Black: return "Black";
            default:                  return "None";
        }
    }
}

void GameSession::tick(int ms) {
    engine_.wait(ms);

    nlohmann::json payload = GameSnapshotCodec::encode(engine_.snapshot());
    if (disconnectStatus_.active) {
        payload["disconnect"] = {
            {"active", true},
            {"color", colorName(disconnectStatus_.color)},
            {"secondsRemaining", disconnectStatus_.secondsRemaining},
        };
    }
    bus_.publish(kSnapshotTopic, payload);
}
