#pragma once
#include <string>
#include <vector>

// The room directory: "which shard hosts room X" - written by a shard
// (SessionRegistry, on room create/destroy) and read by the Gateway (to
// decide which shard to route a JOIN_ROOM to). Lives under shared/ rather
// than server/db/ because both binaries need it, unlike UserRepository
// (server-only). Only RedisRoomDirectory implements this today
// (Docker/Linux build only). A null IRoomDirectory* on the write side (the
// default everywhere, including every existing SessionRegistry-constructing
// test) makes set/erase no-ops - see SessionRegistry.h.
class IRoomDirectory {
public:
    virtual ~IRoomDirectory() = default;

    virtual void set(const std::string& roomName, const std::string& shardAddress) = 0;
    virtual void erase(const std::string& roomName) = 0;

    // "" if roomName has no entry (room doesn't exist, or was never
    // registered - e.g. the native Windows/no-directory build).
    virtual std::string get(const std::string& roomName) const = 0;

    // Every room name currently registered - backs the API Gateway's
    // GET /rooms (the diagram's "Rooms API" listing responsibility).
    virtual std::vector<std::string> list() const = 0;
};
