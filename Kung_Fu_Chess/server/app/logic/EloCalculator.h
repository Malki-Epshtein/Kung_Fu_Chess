#pragma once
#include <utility>

// Pure function, no game/network knowledge - the standard Elo formula
// (logistic expected-score curve, K-factor 32, the common default for
// general online multiplayer matchmaking). Doesn't need to know *why*
// someone won (king capture vs. disconnect) - a win is a win for rating
// purposes, same as real Elo-based systems.
class EloCalculator {
public:
    static constexpr int kFactor = 32;

    // Returns {newWinnerElo, newLoserElo}.
    static std::pair<int, int> apply(int winnerElo, int loserElo);
};
