#pragma once
#include "../../../shared/bus/EventBus.h"
#include <string>

class IGameStateStore;
class SessionRegistry;

// Another independent subscriber alongside EloService/GameHistoryService -
// same extension point EloService's own header comment describes. Unlike
// those two (which react once, at gameEndedTopic), this one mirrors the
// room's current board state + move log to Redis once per completed move
// (moveLogTopic), for crash forensics only - see IGameStateStore.h. Needs a
// SessionRegistry& (unlike EloService/GameHistoryService) to look up the
// live GameSession for its current state at the moment of the event, since
// moveLogTopic's own payload only carries the single move that just
// completed, not the room's full state.
class GameStateMirrorService {
public:
    GameStateMirrorService(IGameStateStore* store, SessionRegistry& registry)
        : store_(store), registry_(registry) {}

    // Subscribes this service to `roomName`'s moveLogTopic/gameEndedTopic on
    // `bus` - called once per room, at the same point EloService::attach/
    // GameHistoryService::attach are.
    void attach(EventBus& bus, const std::string& roomName);

private:
    // roomName isn't part of moveLogTopic's payload (a single MoveEntry has
    // no notion of which room it belongs to) - the subscribing lambdas in
    // attach() capture it per-room, same as GameHistoryService::attach does.
    void mirror(const std::string& roomName);

    IGameStateStore* store_;
    SessionRegistry& registry_;
};
