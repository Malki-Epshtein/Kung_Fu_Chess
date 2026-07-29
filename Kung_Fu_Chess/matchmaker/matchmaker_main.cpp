#include "../shared/bus/NatsClient.h"
#include "../shared/matchmaking/RedisMatchPool.h"
#include "../shared/log/Log.h"
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <set>
#include <string>
#include <thread>

namespace {
    // How long a ticket waits in the pool before giving up - same value
    // and meaning as the old in-process FindGameHandler's kSearchTimeoutSeconds.
    constexpr int kSearchTimeoutSeconds = 60;

    // Same value the old in-memory Matchmaker::kEloRange used (see
    // shared/matchmaking/Matchmaker.h) - kept here now that pairing no
    // longer goes through that class.
    constexpr int kEloRange = 100;

    // How long a just-matched pair waits for GameAllocator to actually
    // assign them a room before this service gives up on their behalf.
    // Once matched, neither ticket has any other timeout watching it - the
    // popped ticket's own kSearchTimeoutSeconds pool timer (if it still has
    // one pending) just silently no-ops via pool.isWaiting(), and the
    // freshly-arrived ticket never had one. Without this, an unreachable
    // GameAllocator (or one whose shard.*.allocate call fails - see
    // allocator_main.cpp's own "dropping match" log line) leaves both
    // players hung forever with no GAME_FOUND and no error. Must exceed
    // GameAllocator's own kAllocateTimeoutMs (5s) plus NATS round-trip
    // slack; far shorter than kSearchTimeoutSeconds since this is only
    // "wait for an already-found opponent to be seated," not "wait to find
    // one at all."
    constexpr int kAssignmentTimeoutSeconds = 10;

    std::string envString(const char* name, const std::string& fallback) {
        const char* value = std::getenv(name);
        return value ? std::string(value) : fallback;
    }
}

int main() {
    Log::init("matchmaker");
    std::string natsUrl = envString("NATS_URL", "nats://localhost:4222");

    // Unlike the old in-memory Matchmaker (pure pairing, no persistence),
    // this process now has no reason to exist without Redis - the waiting
    // pool has to be shared across every Matchmaker replica for
    // subscribeQueue's "exactly one replica handles each request" to
    // actually be safe (see RedisMatchPool.h). An in-memory fallback here
    // would silently reintroduce the exact cross-replica race this move to
    // Redis exists to close, the moment someone actually scales replicas -
    // same fail-fast reasoning as game-allocator/api-gateway's own hard
    // dependencies.
    const char* redisHost = std::getenv("REDIS_HOST");
    if (!redisHost) {
        spdlog::error("matchmaker: REDIS_HOST not set, nothing to share the waiting pool with");
        return 1;
    }
    const char* redisPortEnv = std::getenv("REDIS_PORT");
    int redisPort = redisPortEnv ? std::atoi(redisPortEnv) : 6379;

    try {
        NatsClient nats(natsUrl);
        RedisMatchPool pool(redisHost, redisPort);

        // Adds ticketId to the pool and arms its 60s timeout - shared by
        // both the normal "no match right now" path and the rare fallback
        // below where a would-be opponent's info already vanished.
        auto waitForMatch = [&](const std::string& ticketId, const std::string& username, int elo) {
            pool.addToPool(ticketId, username, elo);
            std::thread([&nats, &pool, ticketId] {
                std::this_thread::sleep_for(std::chrono::seconds(kSearchTimeoutSeconds));
                if (!pool.isWaiting(ticketId))
                    return; // matched or cancelled in the meantime - this firing is moot
                pool.remove(ticketId);
                nats.publish("matchmaking.timeout", { {"ticketId", ticketId} });
                spdlog::info("ticket {} timed out, no match found", ticketId);
            }).detach();
        };

        // Ticket IDs currently matched and waiting on GameAllocator to
        // publish matchmaking.assigned for them - see kAssignmentTimeoutSeconds
        // above. Local to this replica: only the one that actually found the
        // match (and so knows both ticket IDs) arms anything here: another
        // replica's own subscriber below just finds nothing to erase, the
        // same harmless no-op shape ApiGateway/FindGameHandler already use
        // for this same subject.
        std::mutex pendingAssignmentMutex;
        std::set<std::string> pendingAssignment;

        // Cancels the watchdog below once GameAllocator actually delivers -
        // fan-out (not a queue group), since every Matchmaker replica must
        // check its own pendingAssignment, exactly like ApiGateway.cpp/
        // FindGameHandler.cpp already do for this identical subject.
        nats.subscribe("matchmaking.assigned", [&](const nlohmann::json& event) {
            std::lock_guard<std::mutex> lock(pendingAssignmentMutex);
            pendingAssignment.erase(event.at("ticketA").get<std::string>());
            pendingAssignment.erase(event.at("ticketB").get<std::string>());
        });

        // Arms the matched-but-not-yet-assigned watchdog for both tickets of
        // a pair this replica just matched - see kAssignmentTimeoutSeconds.
        auto watchForAssignment = [&](const std::string& ticketA, const std::string& ticketB) {
            {
                std::lock_guard<std::mutex> lock(pendingAssignmentMutex);
                pendingAssignment.insert(ticketA);
                pendingAssignment.insert(ticketB);
            }
            std::thread([&nats, &pendingAssignmentMutex, &pendingAssignment, ticketA, ticketB] {
                std::this_thread::sleep_for(std::chrono::seconds(kAssignmentTimeoutSeconds));
                std::lock_guard<std::mutex> lock(pendingAssignmentMutex);
                for (const std::string& ticketId : { ticketA, ticketB }) {
                    if (pendingAssignment.erase(ticketId) == 0)
                        continue; // matchmaking.assigned already cancelled this one
                    nats.publish("matchmaking.timeout", { {"ticketId", ticketId} });
                    spdlog::info("ticket {} timed out waiting for room assignment "
                                 "(GameAllocator unreachable or allocation failed)", ticketId);
                }
            }).detach();
        };

        // Queue group ("matchmakers") - exactly one Matchmaker replica
        // handles each request, not all of them. Still required for
        // correctness even with a shared Redis pool: without it, every
        // replica would independently call findMatch() for the *same*
        // incoming ticket, and two of them finding two different opponents
        // before either publishes would mean two conflicting
        // matchmaking.matched events for the same ticket (see
        // RedisMatchPool.h and INatsClient::subscribeQueue's own comments).
        nats.subscribeQueue("matchmaking.request", "matchmakers", [&](const nlohmann::json& request) {
            std::string ticketId = request.at("ticketId").get<std::string>();
            std::string username = request.at("username").get<std::string>();
            int elo = request.at("elo").get<int>();

            std::optional<MatchCandidate> opponent = pool.findMatch(elo, kEloRange);
            if (opponent) {
                spdlog::info("matched '{}' (ticket {}) with '{}' (ticket {})",
                             opponent->username, opponent->ticketId, username, ticketId);
                nats.publish("matchmaking.matched",
                              { {"ticketA", opponent->ticketId}, {"usernameA", opponent->username}, {"eloA", opponent->elo},
                                {"ticketB", ticketId}, {"usernameB", username}, {"eloB", elo} });
                watchForAssignment(opponent->ticketId, ticketId);
                return;
            }

            waitForMatch(ticketId, username, elo);
        });

        // Plain fan-out subscribe, not a queue group - remove() is
        // idempotent (ZREM/DEL on an already-gone ticket are no-ops), so
        // every replica trying the same removal is harmless; only the
        // first one to run actually finds anything to remove.
        nats.subscribe("matchmaking.cancel", [&](const nlohmann::json& event) {
            std::string ticketId = event.at("ticketId").get<std::string>();
            pool.remove(ticketId);
        });

        spdlog::info("matchmaker running, connected to {}", natsUrl);
        while (true)
            std::this_thread::sleep_for(std::chrono::hours(1));
    } catch (const std::exception& e) {
        spdlog::error("matchmaker failed to start: {}", e.what());
        return 1;
    }
    return 0;
}
