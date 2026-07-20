#include "Matchmaker.h"
#include <cstdlib>

void Matchmaker::addToPool(ConnectionHandle hdl, int elo) {
    pool_[hdl] = elo;
}

void Matchmaker::remove(ConnectionHandle hdl) {
    pool_.erase(hdl);
}

std::optional<Matchmaker::ConnectionHandle> Matchmaker::findMatch(int elo) const {
    for (const auto& [hdl, waitingElo] : pool_) {
        if (std::abs(waitingElo - elo) <= kEloRange)
            return hdl;
    }
    return std::nullopt;
}

bool Matchmaker::isWaiting(ConnectionHandle hdl) const {
    return pool_.count(hdl) != 0;
}
