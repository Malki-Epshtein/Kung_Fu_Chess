#include "EloCalculator.h"
#include <cmath>

std::pair<int, int> EloCalculator::apply(int winnerElo, int loserElo) {
    double expectedWinner = 1.0 / (1.0 + std::pow(10.0, (loserElo - winnerElo) / 400.0));
    double expectedLoser  = 1.0 - expectedWinner;

    int newWinnerElo = winnerElo + static_cast<int>(std::lround(kFactor * (1.0 - expectedWinner)));
    int newLoserElo  = loserElo  + static_cast<int>(std::lround(kFactor * (0.0 - expectedLoser)));

    return { newWinnerElo, newLoserElo };
}
