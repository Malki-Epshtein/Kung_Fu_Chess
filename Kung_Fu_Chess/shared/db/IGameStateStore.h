#pragma once
#include <string>

// Common surface for whatever mirrors a room's live, in-progress game state -
// today that's only RedisGameStateStore (Docker/Linux build only, selected
// at runtime via REDIS_HOST, same as RedisReconnectStore - see
// server_main.cpp). A null IGameStateStore* (GameStateMirrorService's
// default) just means this mirror is skipped entirely, same graceful-degrade
// shape every other optional dependency in this codebase already uses.
//
// Documentation only - a shard crash still loses the live GameSession
// entirely; nothing reads this back to reconstruct or resume a room. This is
// a *live, temporary* mirror of a game still in progress, separate from
// GameHistoryService/Postgres, which durably records a game only once it has
// actually finished.
class IGameStateStore {
public:
    virtual ~IGameStateStore() = default;

    // Called once per completed move (see GameStateMirrorService) -
    // overwrites whatever was there before with the room's current state.
    virtual void save(const std::string& roomName, const std::string& boardStateJson,
                       const std::string& moveLogJson, const std::string& whiteUsername, int whiteElo,
                       const std::string& blackUsername, int blackElo) = 0;

    // Called once the game ends normally (gameEndedTopic) - the live mirror
    // has no further reason to exist once GameHistoryService has durably
    // recorded the real result. No-op if nothing was stored for this room.
    virtual void clear(const std::string& roomName) = 0;
};
