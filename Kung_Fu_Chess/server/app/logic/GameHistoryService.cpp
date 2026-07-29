#include "GameHistoryService.h"
#include "../session/GameSession.h"
#include "../session/GameResultCodec.h"
#include "../../../shared/db/IGameHistoryRepository.h"

void GameHistoryService::attach(EventBus& bus, const std::string& roomName) {
    bus.subscribe(GameSession::gameEndedTopic(roomName), [this, roomName](const nlohmann::json& data) {
        onGameEnded(roomName, data);
    });
}

void GameHistoryService::onGameEnded(const std::string& roomName, const nlohmann::json& data) {
    if (!repository_)
        return; // no Postgres configured - nothing to record to

    GameResult result = GameResultCodec::decode(data);
    if (result.winnerUsername.empty() || result.loserUsername.empty())
        return; // one seat was never filled - nothing real to record, same guard EloService uses

    repository_->record(GameRecord{
        roomName,
        result.winnerUsername, result.winnerElo,
        result.loserUsername, result.loserElo,
        result.reason == GameEndReason::KingCapture ? "KingCapture" : "Disconnect",
        result.moveLog,
    });
}
