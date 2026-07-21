#include "GameSession.h"
#include "../../../shared/protocol/GameSnapshotCodec.h"
#include "../../../shared/protocol/MoveLogCodec.h"
#include "../../../shared/protocol/MessageCodec.h"

namespace {
    const char* colorName(Chess::Color color) {
        switch (color) {
            case Chess::Color::White: return "White";
            case Chess::Color::Black: return "Black";
            default:                  return "None";
        }
    }
}

GameSession::GameSession(std::shared_ptr<Board> board, EventBus& bus, std::string roomName, bool simultaneousMode)
    : engine_(board, simultaneousMode), bus_(bus), roomName_(std::move(roomName)), moveLogObserver_(board->getHeight()) {
    engine_.addCaptureObserver(&scoreObserver_);
    engine_.addMoveObserver(&moveLogObserver_);

    // Bridges MoveLogObserver into the network layer without it ever
    // knowing EventBus/JSON exist - see MoveLogObserver::onNewEntry.
    // Wrapped in the standard Message envelope (like GameFound) so the
    // client can tell a MOVE_LOGGED push apart from a plain snapshot -
    // same parse(encode(...)) round-trip FindGameHandler already uses to
    // turn a Message into the json a bus/reply actually carries.
    moveLogObserver_.onNewEntry = [this](Chess::Color color, const MoveEntry& entry) {
        Message msg{ MessageType::MoveLogged, MoveLogCodec::encode(color, entry) };
        bus_.publish(moveLogTopic(roomName_), nlohmann::json::parse(MessageCodec::encode(msg)));
    };
}

nlohmann::json GameSession::fullMoveLog() const {
    return MoveLogCodec::encodeAll(moveLogObserver_.getMoves(Chess::Color::White), moveLogObserver_.getMoves(Chess::Color::Black));
}

void GameSession::tick(int ms) {
    engine_.wait(ms);

    nlohmann::json payload = GameSnapshotCodec::encode(engine_.snapshot());
    payload["score"] = {
        {"white", scoreObserver_.getScore(Chess::Color::White)},//זב כן מספיק בSNAPSHOT
        {"black", scoreObserver_.getScore(Chess::Color::Black)},
    };
    if (disconnectStatus_.active) {
        payload["disconnect"] = {
            {"active", true},
            {"color", colorName(disconnectStatus_.color)},
            {"secondsRemaining", disconnectStatus_.secondsRemaining},
        };
    }
    bus_.publish(snapshotTopic(roomName_), payload);
}
