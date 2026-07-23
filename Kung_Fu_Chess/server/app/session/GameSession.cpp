#include "GameSession.h"
#include "GameResultCodec.h"
#include "../../../shared/protocol/GameSnapshotCodec.h"
#include "../../../shared/protocol/MoveLogCodec.h"
#include "../../../shared/protocol/CaptureEventCodec.h"
#include "../../../shared/protocol/MessageCodec.h"

namespace {
    const char* colorName(Chess::Color color) {
        switch (color) {
            case Chess::Color::White: return "White";
            case Chess::Color::Black: return "Black";
            default:                  return "None";
        }
    }

    // A king is only ever missing from the snapshot once it's been
    // captured, so whichever color's king is still present is the winner.
    // Deliberately duplicated from ImageView.cpp's identical helper (same
    // convention as this codebase's per-file kKindToName/kColorToName maps)
    // rather than shared, since client/server sharing a header here would
    // be the only cross-boundary include in either direction.
    Chess::Color survivingKingColor(const GameSnapshot& snapshot) {
        bool whiteKing = false, blackKing = false;
        for (const auto& piece : snapshot.pieces) {
            if (piece.kind != Chess::Kind::King) continue;
            if (piece.color == Chess::Color::White) whiteKing = true;
            else if (piece.color == Chess::Color::Black) blackKing = true;
        }
        if (whiteKing && !blackKing) return Chess::Color::White;
        if (blackKing && !whiteKing) return Chess::Color::Black;
        return Chess::Color::None;
    }
}

GameSession::GameSession(std::shared_ptr<Board> board, EventBus& bus, std::string roomName, bool simultaneousMode)
    : engine_(board, simultaneousMode), bus_(bus), roomName_(std::move(roomName)), moveLogObserver_(board->getHeight()) {
    engine_.addCaptureObserver(&scoreObserver_);
    engine_.addMoveObserver(&moveLogObserver_);
    engine_.addCaptureObserver(&captureEventObserver_);

    // Bridges MoveLogObserver into the network layer without it ever
    // knowing EventBus/JSON exist - see MoveLogObserver::onNewEntry.
    // Wrapped in the standard Message envelope (like GameFound) so the
    // client can tell a MOVE_LOGGED push apart from a plain snapshot -
    // same parse(encode(...)) round-trip FindGameHandler already uses to
    // turn a Message into the json a bus/reply actually carries.
    //
    // Enqueues into pending_ instead of publishing directly: this callback
    // fires synchronously from inside engine_.wait() (called by
    // computeStep(), which may run on a worker thread once WsServer starts
    // using the thread pool) - EventBus::publish/subscribe are not
    // synchronized, so the actual bus_.publish() call is deferred to
    // publishStep(), which only ever runs on the main thread.
    moveLogObserver_.onNewEntry = [this](Chess::Color color, const MoveEntry& entry) {
        Message msg{ MessageType::MoveLogged, MoveLogCodec::encode(color, entry) };
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pending_.push_back({ moveLogTopic(roomName_), nlohmann::json::parse(MessageCodec::encode(msg)) });
    };

    // Same bridging pattern as moveLogObserver_.onNewEntry above, for a
    // capture instead of a completed move.
    captureEventObserver_.onNewCapture = [this](const CaptureEvent& event) {
        Message msg{ MessageType::CaptureEvent, CaptureEventCodec::encode(event) };
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pending_.push_back({ captureTopic(roomName_), nlohmann::json::parse(MessageCodec::encode(msg)) });
    };
}

void GameSession::markDisconnectResign(Chess::Color loser, std::string loserUsername, int loserElo) {
    // Also checks the live engine state directly, not just
    // gameResultPublished_ - a king capture can make
    // engine_.snapshot().game_over true on this same tick, just before
    // this fires from the disconnect timer. Without this check, a
    // disconnect landing in that window could publish a second, wrong
    // result right after the real one.
    if (gameResultPublished_ || engine_.snapshot().game_over)
        return; // already ended some other way - first result wins

    bool winnerIsWhite = (loser != Chess::Color::White);
    Chess::Color winner = winnerIsWhite ? Chess::Color::White : Chess::Color::Black;
    publishGameEnded(GameResult{
        winner, GameEndReason::Disconnect,
        winnerIsWhite ? identity_.whiteUsername : identity_.blackUsername,
        winnerIsWhite ? identity_.whiteElo : identity_.blackElo,
        std::move(loserUsername), loserElo,
    });
}

void GameSession::publishGameEnded(const GameResult& result) {//פה אני מפיצה את התוצאת המשחקת על הTOPIC של המשחק שהסתיים, ומסמנת שהמשחק כבר הסתיים
    gameResultPublished_ = true;
    bus_.publish(gameEndedTopic(roomName_), GameResultCodec::encode(result));
}

nlohmann::json GameSession::fullMoveLog() const {
    return MoveLogCodec::encodeAll(moveLogObserver_.getMoves(Chess::Color::White), moveLogObserver_.getMoves(Chess::Color::Black));
}

nlohmann::json GameSession::computeStep(int ms) {
    engine_.wait(ms);

    nlohmann::json payload = GameSnapshotCodec::encode(engine_.snapshot());
    // Tagged (flat, not re-enveloped under "payload" - every existing
    // field/consumer of this broadcast stays as-is) so the client can tell
    // it apart from a command-ack reply without guessing from field
    // presence - see MessageCodec::typeName.
    payload["type"] = MessageCodec::typeName(MessageType::Snapshot);
    payload["score"] = {
        {"white", scoreObserver_.getScore(Chess::Color::White)},//זב כן מספיק בSNAPSHOT
        {"black", scoreObserver_.getScore(Chess::Color::Black)},
    };
    payload["identity"] = {
        {"whiteName", identity_.whiteUsername}, {"whiteElo", identity_.whiteElo},
        {"blackName", identity_.blackUsername}, {"blackElo", identity_.blackElo},
        {"spectatorCount", identity_.spectatorCount},
    };
    if (disconnectStatus_.active) {
        payload["disconnect"] = {
            {"active", true},
            {"color", colorName(disconnectStatus_.color)},
            {"secondsRemaining", disconnectStatus_.secondsRemaining},
        };
    }
    return payload;
}

void GameSession::publishStep(const nlohmann::json& snapshotPayload) {
    // Drain whatever moveLogObserver_/captureEventObserver_ queued during
    // the computeStep() that just ran (see their onNewEntry/onNewCapture
    // lambdas in the constructor) - this is the one place guaranteed to run
    // on the main thread, so it's the only place that actually touches
    // EventBus for these two.
    std::vector<PendingPublish> pending;
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pending.swap(pending_);
    }
    for (const auto& p : pending)
        bus_.publish(p.topic, p.data);

    // Direct delivery (see setSnapshotSender/class comment) instead of
    // EventBus - this is continuously-changing state, not a one-off event.
    // .dump() matches what NetworkBroadcaster does internally for
    // moveLogTopic/captureTopic above, so the wire format is unaffected.
    if (snapshotSender_)
        snapshotSender_(snapshotPayload.dump());

    // King-capture game-over detection rides on the same engine_.snapshot()
    // computeStep() already advanced above - checking here costs nothing
    // extra, and publishes gameEndedTopic the moment it's first true
    // (edge-triggered via gameResultPublished_), the same tick clients see
    // game_over:true on the snapshot. A disconnect-caused ending instead
    // reaches gameEndedTopic via markDisconnectResign, called directly by
    // SessionRegistry's timer - not from here.
    if (!gameResultPublished_ && engine_.snapshot().game_over) {
        Chess::Color winner = survivingKingColor(engine_.snapshot());
        bool winnerIsWhite = winner == Chess::Color::White;
        publishGameEnded(GameResult{
            winner, GameEndReason::KingCapture,
            winnerIsWhite ? identity_.whiteUsername : identity_.blackUsername,
            winnerIsWhite ? identity_.whiteElo : identity_.blackElo,
            winnerIsWhite ? identity_.blackUsername : identity_.whiteUsername,
            winnerIsWhite ? identity_.blackElo : identity_.whiteElo,
        });
    }
}
