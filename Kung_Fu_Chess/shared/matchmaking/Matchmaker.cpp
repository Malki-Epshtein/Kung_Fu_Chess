#include "Matchmaker.h"
#include <cstdlib>

void Matchmaker::addToPool(const std::string& ticketId, int elo) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_[ticketId] = elo;
}

void Matchmaker::remove(const std::string& ticketId) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.erase(ticketId);
}

std::optional<std::string> Matchmaker::findMatch(int elo) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [ticketId, waitingElo] : pool_) {
        if (std::abs(waitingElo - elo) <= kEloRange)
            return ticketId;
    }
    return std::nullopt;
}

bool Matchmaker::isWaiting(const std::string& ticketId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pool_.count(ticketId) != 0;
}
