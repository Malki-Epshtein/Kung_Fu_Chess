#include "../shared/bus/NatsClient.h"
#include "../shared/db/RedisSequence.h"
#include "../shared/log/Log.h"
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace {
    std::string envString(const char* name, const std::string& fallback) {
        const char* value = std::getenv(name);
        return value ? std::string(value) : fallback;
    }

    // Must match MessageDispatcher.cpp's identical transform - both sides
    // independently sanitize the same raw SHARD_ADDRESS into the same NATS
    // subject token, without coordinating anything else.
    std::string subjectSafe(const std::string& raw) {
        std::string safe = raw;
        for (char& c : safe)
            if (!std::isalnum(static_cast<unsigned char>(c)))
                c = '_';
        return safe;
    }

    constexpr int kAllocateTimeoutMs = 5000;
}

int main() {
    Log::init("game-allocator");
    std::string natsUrl = envString("NATS_URL", "nats://localhost:4222");

    // Unlike the Matchmaker service (pure pairing, no persistence), this
    // process has no reason to exist without Redis - room names are minted
    // from a shared counter so they stay unique across every shard (see
    // RedisSequence's header for the bug this fixes). Fail fast rather
    // than start a process that can never actually allocate anything.
    const char* redisHost = std::getenv("REDIS_HOST");
    if (!redisHost) {
        spdlog::error("game-allocator: REDIS_HOST not set, nothing to allocate room names with");
        return 1;
    }
    const char* redisPortEnv = std::getenv("REDIS_PORT");
    int redisPort = redisPortEnv ? std::atoi(redisPortEnv) : 6379;

    try {
        NatsClient nats(natsUrl);
        RedisSequence roomNames(redisHost, redisPort);

        // shardAddress -> its last-reported room count (server_main.cpp's
        // startHeartbeat publishes this every 5s) - "least loaded" just
        // means the lowest value seen here. A shard that's never sent a
        // heartbeat yet is invisible rather than assumed idle, so a
        // just-started shard doesn't get flooded before its first beat.
        std::mutex shardLoadMutex;
        std::unordered_map<std::string, long long> shardLoad;

        // Deliberately still plain fan-out, not a queue group - every
        // replica needs its own view of shard load to make its own
        // least-loaded decision; there's no single owner to hand this off
        // to the way matchmaking.matched/allocation.request have below.
        nats.subscribe("shard.heartbeat", [&](const nlohmann::json& event) {
            std::string shard = event.at("shard").get<std::string>();
            long long roomCount = event.at("roomCount").get<long long>();
            std::lock_guard<std::mutex> lock(shardLoadMutex);
            shardLoad[shard] = roomCount;
        });

        // Shared by both subscriptions below - "least loaded" just means
        // the lowest roomCount seen in shardLoad. Returns "" if no shard
        // has ever sent a heartbeat.
        auto pickLeastLoadedShard = [&]() -> std::string {
            std::string chosenShard;
            std::lock_guard<std::mutex> lock(shardLoadMutex);
            long long best = std::numeric_limits<long long>::max();
            for (const auto& [shard, roomCount] : shardLoad) {
                if (roomCount < best) {
                    best = roomCount;
                    chosenShard = shard;
                }
            }
            return chosenShard;
        };

        // Queue group ("allocators") - exactly one GameAllocator replica
        // handles each matched pair, not all of them (see
        // INatsClient::subscribeQueue's own comment). Without this, 2+
        // replicas would each independently allocate a *second* real room
        // for the same match.
        nats.subscribeQueue("matchmaking.matched", "allocators", [&](const nlohmann::json& event) {
            std::string chosenShard = pickLeastLoadedShard();
            if (chosenShard.empty()) {
                spdlog::error("no shard heartbeat seen yet - dropping match (both players will time out)");
                return;
            }

            std::string roomName = "match-" + std::to_string(roomNames.next("matchmaking:nextRoomId"));
            std::string whiteUsername = event.at("usernameA").get<std::string>(); // was waiting first -> White
            std::string blackUsername = event.at("usernameB").get<std::string>(); // just matched -> Black

            // Synchronous by design (NATS request/reply) - blocks this
            // subscription's delivery thread until the shard replies or
            // times out. No retry against a different shard on failure -
            // out of scope for this pass; a dropped match here just leaves
            // both tickets to their client-side timeout/retry.
            std::optional<nlohmann::json> reply = nats.request(
                "shard." + subjectSafe(chosenShard) + ".allocate",
                { {"roomName", roomName}, {"whiteUsername", whiteUsername}, {"blackUsername", blackUsername} },
                kAllocateTimeoutMs);

            if (!reply || !reply->value("success", false)) {
                spdlog::error("shard {} failed to allocate room '{}' - dropping match", chosenShard, roomName);
                return;
            }

            nats.publish("matchmaking.assigned",
                          { {"ticketA", event.at("ticketA")}, {"roleA", "White"},
                            {"ticketB", event.at("ticketB")}, {"roleB", "Black"},
                            {"roomName", roomName}, {"shard", chosenShard},
                            {"whiteName", whiteUsername}, {"whiteElo", event.at("eloA")},
                            {"blackName", blackUsername}, {"blackElo", event.at("eloB")} });
            spdlog::info("allocated '{}' on {} for '{}' vs '{}'", roomName, chosenShard, whiteUsername, blackUsername);
        });

        // The Rooms API's counterpart to matchmaking.matched above - the
        // API Gateway's POST /rooms sends this instead of picking a shard
        // itself (see ApiGateway.cpp), so a regular room's shard is decided
        // by the same least-loaded logic as a matched PLAY game, not
        // round-robin. The room name here is the one the user typed -
        // unlike matchmaking.matched, nothing is minted from roomNames/
        // RedisSequence. No whiteUsername/blackUsername is sent to
        // AllocateRoomHandler, so no seats are reserved - the creator
        // claims White by arrival order once they connect via EnterRoom.
        // Same queue-group reasoning as matchmaking.matched above -
        // otherwise 2+ replicas would each independently answer the same
        // request, and while the caller only sees whichever reply wins the
        // race, every replica's own shard.<addr>.allocate call still ran,
        // leaving orphaned rooms behind.
        nats.subscribeRequestQueue("allocation.request", "allocators", [&](const nlohmann::json& request) -> nlohmann::json {
            std::string roomName = request.at("roomName").get<std::string>();

            std::string chosenShard = pickLeastLoadedShard();
            if (chosenShard.empty())
                return { {"success", false}, {"message", "no game-server shard available"} };

            std::optional<nlohmann::json> reply = nats.request(
                "shard." + subjectSafe(chosenShard) + ".allocate", { {"roomName", roomName} }, kAllocateTimeoutMs);

            if (!reply)
                return { {"success", false}, {"message", "shard unavailable"} };
            if (!reply->value("success", false))
                return *reply; // e.g. "room already exists" - pass the shard's own message through

            spdlog::info("allocated room '{}' on {}", roomName, chosenShard);
            return { {"success", true}, {"message", "room created"}, {"roomName", roomName}, {"shard", chosenShard} };
        });

        spdlog::info("game-allocator running, connected to {}", natsUrl);
        while (true)
            std::this_thread::sleep_for(std::chrono::hours(1));
    } catch (const std::exception& e) {
        spdlog::error("game-allocator failed to start: {}", e.what());
        return 1;
    }
    return 0;
}
