#pragma once
#include "../../../shared/bus/EventBus.h"
#include <string>

class IGameHistoryRepository;

// Another independent subscriber to gameEndedTopic, alongside EloService -
// exactly the extension point EloService's own header comment describes
// ("match history... would be another independent subscriber to the same
// topic, never touching this class, GameSession, or WsServer's tick loop").
// Unlike EloService's required IUserRepository&, the repository here is a
// nullable pointer: a `users` table always exists in every build, but
// Postgres game history doesn't (native Windows, or no DATABASE_URL) - see
// IGameHistoryRepository.h.
class GameHistoryService {
public:
    explicit GameHistoryService(IGameHistoryRepository* repository) : repository_(repository) {}

    // Subscribes this service to `roomName`'s gameEndedTopic on `bus` -
    // called once per room, at the same point EloService::attach is.
    void attach(EventBus& bus, const std::string& roomName);

private:
    // roomName isn't part of the gameEndedTopic payload itself (GameResult
    // has no notion of which room it belongs to) - the subscribing lambda
    // in attach() captures it per-room, same as EloService's attach() would
    // need to if it ever wanted the room name too.
    void onGameEnded(const std::string& roomName, const nlohmann::json& data);

    IGameHistoryRepository* repository_;
};
