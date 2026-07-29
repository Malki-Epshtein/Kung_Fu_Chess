#pragma once
#include <map>
#include <mutex>
#include <optional>
#include <string>

// The ELO-based waiting pool - pairing logic unchanged from its original
// in-process version (see git history: server/app/logic/Matchmaker), just
// re-keyed off a portable `ticketId` instead of a process-local
// connection_hdl, since this now runs inside the standalone Matchmaker
// service (Server_Design.md step 6) reachable by any shard over NATS
// rather than living inside one shard's process. Thread-safe (unlike the
// original): the service isn't pinned to a single io thread the way the
// in-process instance was, so every method locks mutex_.
class Matchmaker {
public:
    static constexpr int kEloRange = 100;

    void addToPool(const std::string& ticketId, int elo);

    // No-op if ticketId isn't in the pool - used both when a match is
    // found and when a waiting ticket is cancelled/times out.
    void remove(const std::string& ticketId);

    // Any waiting ticket within kEloRange of `elo` - nullopt if the pool is
    // empty or nobody currently waiting qualifies.
    std::optional<std::string> findMatch(int elo) const;

    // True while ticketId is still sitting in the pool (added, not yet
    // matched or removed).
    bool isWaiting(const std::string& ticketId) const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, int> pool_;
};
