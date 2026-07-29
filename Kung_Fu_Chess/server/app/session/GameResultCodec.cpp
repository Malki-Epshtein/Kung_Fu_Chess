#include "GameResultCodec.h"
#include <stdexcept>
#include <unordered_map>

namespace {
    const std::unordered_map<Chess::Color, std::string> kColorToName = {
        { Chess::Color::White, "White" }, { Chess::Color::Black, "Black" },
    };
    Chess::Color nameToColor(const std::string& name) {
        for (const auto& [color, colorName] : kColorToName)
            if (colorName == name) return color;
        throw std::runtime_error("Unknown color: " + name);
    }

    const std::unordered_map<GameEndReason, std::string> kReasonToName = {
        { GameEndReason::KingCapture, "KingCapture" }, { GameEndReason::Disconnect, "Disconnect" },
    };
    GameEndReason nameToReason(const std::string& name) {
        for (const auto& [reason, reasonName] : kReasonToName)
            if (reasonName == name) return reason;
        throw std::runtime_error("Unknown game end reason: " + name);
    }
}

nlohmann::json GameResultCodec::encode(const GameResult& result) {
    return {
        {"winner", kColorToName.at(result.winner)},
        {"reason", kReasonToName.at(result.reason)},
        {"winnerUsername", result.winnerUsername}, {"winnerElo", result.winnerElo},
        {"loserUsername", result.loserUsername}, {"loserElo", result.loserElo},
        {"moveLog", result.moveLog},
    };
}

GameResult GameResultCodec::decode(const nlohmann::json& j) {
    return GameResult{
        nameToColor(j.at("winner").get<std::string>()),
        nameToReason(j.at("reason").get<std::string>()),
        j.at("winnerUsername").get<std::string>(), j.at("winnerElo").get<int>(),
        j.at("loserUsername").get<std::string>(), j.at("loserElo").get<int>(),
        j.value("moveLog", nlohmann::json::object()),
    };
}
