#pragma once
#include <string>

// Common surface for whatever stores a room's live disconnect-grace-period
// status - today that's only RedisReconnectStore (Docker/Linux build only,
// selected at runtime via REDIS_HOST, same as RedisRoomDirectory - see
// server_main.cpp). A null IReconnectStore* (SessionRegistry's default)
// just means this status is only ever in-memory, as it always was before
// this existed - the diagram's Redis "reconnect" box, made real.
//
// Documentation only - a shard crash still loses the live GameSession
// entirely; nothing reads this back to reconstruct or resume a room.
class IReconnectStore {
public:
    virtual ~IReconnectStore() = default;

    // Called once a second while `roomName`'s disconnect countdown is
    // running (see SessionRegistry::startDisconnectCountdown) - overwrites
    // whatever was there before.
    virtual void setDisconnected(const std::string& roomName, const std::string& color,
                                  const std::string& username, int elo, int secondsRemaining) = 0;

    // Called once the countdown resolves (auto-resign). No-op if nothing
    // was stored for this room.
    virtual void clearDisconnected(const std::string& roomName) = 0;
};
