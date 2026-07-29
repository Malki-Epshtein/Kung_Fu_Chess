#pragma once
#include "json.hpp"
#include <string>
#include <vector>

// One finished game, decoupled from the session layer's GameResult
// (Chess::Color/GameEndReason) on purpose - ApiGateway links no game/model
// code at all, so this repository interface (used by both Server, which
// writes, and ApiGateway, which reads for GET /history) has to stay
// engine-independent. GameHistoryService is what translates a real
// GameResult into this DTO before calling record().
struct GameRecord {
    std::string roomName;
    std::string winnerUsername;
    int         winnerElo = 0;
    std::string loserUsername;
    int         loserElo = 0;
    std::string reason; // "KingCapture" | "Disconnect"
    nlohmann::json moveLog; // MoveLogCodec::encodeAll shape
};

// Common surface for whatever stores finished-game history - today that's
// only PostgresGameHistoryRepository (Docker/Linux build only, selected at
// runtime via DATABASE_URL, same as PostgresUserRepository - see
// server_main.cpp/api_gateway_main.cpp). Unlike IUserRepository, there's no
// SQLite/native-Windows implementation - a null IGameHistoryRepository*
// (GameHistoryService on the write side, ApiGateway's GET /history on the
// read side) just means "no history available", not a build error.
class IGameHistoryRepository {
public:
    virtual ~IGameHistoryRepository() = default;

    virtual void record(const GameRecord& record) = 0;

    // Most recent first, capped at `limit`.
    virtual std::vector<GameRecord> list(int limit) const = 0;
};
