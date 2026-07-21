#pragma once
#include "../../../shared/bus/EventBus.h"
#include <string>

class UserRepository;

// The one thing that reacts to a room's GameSession::gameEndedTopic today
// - applies an Elo update via EloCalculator + UserRepository - but doesn't
// need to be the only one: anything else that should react to "a game
// just ended" (match history, leaderboards, notifications, ...) would be
// another independent subscriber to the same topic, never touching this
// class, GameSession, or WsServer's tick loop.
class EloService {
public:
    explicit EloService(UserRepository& users) : users_(users) {}

    // Subscribes this service to `roomName`'s gameEndedTopic on `bus` -
    // called once per room, at the same point BroadcasterManager attaches
    // its own subscribers for that room.
    void attach(EventBus& bus, const std::string& roomName);

private:
    void onGameEnded(const nlohmann::json& data);

    UserRepository& users_;
};
