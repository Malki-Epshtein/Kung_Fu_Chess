#include "EloService.h"
#include "EloCalculator.h"
#include "../session/GameSession.h"
#include "../session/GameResultCodec.h"
#include "../../db/UserRepository.h"
#include "../../../shared/log/Log.h"

void EloService::attach(EventBus& bus, const std::string& roomName) {
    bus.subscribe(GameSession::gameEndedTopic(roomName), [this](const nlohmann::json& data) {
        onGameEnded(data);
    });
}

void EloService::onGameEnded(const nlohmann::json& data) {
    GameResult result = GameResultCodec::decode(data);
    if (result.winnerUsername.empty() || result.loserUsername.empty())
        return; // one seat was never filled - nothing real to rate

    auto [newWinnerElo, newLoserElo] = EloCalculator::apply(result.winnerElo, result.loserElo);
    users_.updateElo(result.winnerUsername, newWinnerElo);
    users_.updateElo(result.loserUsername, newLoserElo);
    spdlog::info("ELO updated: {} {}->{}, {} {}->{}", result.winnerUsername, result.winnerElo, newWinnerElo,
                  result.loserUsername, result.loserElo, newLoserElo);
}
