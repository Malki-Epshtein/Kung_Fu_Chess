#pragma once
#include "../bus/INatsClient.h"
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Tracks which game-server shards are currently alive, purely from
// server_main.cpp's periodic "shard.heartbeat" NATS publish - the same
// signal GameAllocator already uses to pick a least-loaded shard (see
// allocator_main.cpp). WsGateway/ApiGateway use this instead of a static,
// hand-enumerated shard list (the old UPSTREAM_URIS env var) so scaling the
// number of shards is purely a replica-count change, not a config edit -
// see docker-compose.yml's gameserver service / the k8s manifests.
class ShardRegistry {
public:
    // Subscribes to shard.heartbeat immediately. staleAfterSeconds should be
    // a small multiple of the shard's own heartbeat interval (5s today - see
    // server_main.cpp's kHeartbeatIntervalSeconds), so a crashed or
    // scaled-down shard drops out of rotation within a few missed beats
    // instead of staying eligible forever - a static list never had this
    // problem because it never expired, but it also never grew.
    explicit ShardRegistry(INatsClient& nats, int staleAfterSeconds = 15);

    // Every shard heard from within staleAfterSeconds - prunes stale
    // entries first. Empty is a valid, expected answer right after startup
    // (before any heartbeat has arrived) or if every shard has gone quiet -
    // callers must handle "no shard to route to yet" rather than assume a
    // non-empty result.
    std::vector<std::string> liveShards();

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> lastSeen_;
    int staleAfterSeconds_;
};
