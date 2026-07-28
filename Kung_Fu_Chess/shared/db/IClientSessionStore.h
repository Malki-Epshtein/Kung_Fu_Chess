#pragma once
#include <optional>
#include <string>

// What a login token resolves to - just enough for ClientSessionRegistry to
// re-establish identity on whatever shard the client's WebSocket ends up on
// (see RedisClientSessionStore's header comment for why this exists).
struct ClientSessionData {
    std::string username;
    int         elo = 0;
};

// The session store: "what user does this token belong to". Written once,
// by the API Gateway, right after a successful HTTP /login (the token is
// minted there and handed to the client). Read by whichever game-server
// shard the client's WebSocket later connects to, via EnterRoomHandler -
// that's the whole point: identity no longer lives only in the memory of
// whichever process happened to run LOGIN, so any shard can resolve it.
// Only RedisClientSessionStore implements this today (Docker/Linux build
// only, same KFC_HAVE_REDIS gating as RedisRoomDirectory).
class IClientSessionStore {
public:
    virtual ~IClientSessionStore() = default;

    virtual void put(const std::string& token, const std::string& username, int elo) = 0;

    // nullopt if the token doesn't exist (never issued, or its TTL expired).
    virtual std::optional<ClientSessionData> get(const std::string& token) const = 0;
};
