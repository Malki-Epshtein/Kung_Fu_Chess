#include "../doctest.h"
#include "../server/app/logic/EloCalculator.h"

TEST_CASE("EloCalculator - שני שחקנים שווים: המנצח עולה בדיוק כמו שהמפסיד יורד") {
    auto [newWinnerElo, newLoserElo] = EloCalculator::apply(1200, 1200);

    CHECK(newWinnerElo == 1216); // K=32 * (1 - 0.5) = 16
    CHECK(newLoserElo == 1184);  // K=32 * (0 - 0.5) = -16
}

TEST_CASE("EloCalculator - מנצח גבוה שמנצח נמוך עולה פחות מ-16") {
    auto [newWinnerElo, newLoserElo] = EloCalculator::apply(1400, 1200);

    CHECK(newWinnerElo < 1416);
    CHECK(newWinnerElo > 1400);
    CHECK(newLoserElo < 1200);
}

TEST_CASE("EloCalculator - מנצח נמוך שמנצח גבוה עולה יותר מ-16 (הפתעה)") {
    auto [newWinnerElo, newLoserElo] = EloCalculator::apply(1200, 1400);

    CHECK(newWinnerElo > 1216);
    CHECK(newLoserElo < 1400);
}

TEST_CASE("EloCalculator - סכום השינויים תמיד מתאזן (מה שהמנצח מרוויח, המפסיד מפסיד)") {
    auto [newWinnerElo, newLoserElo] = EloCalculator::apply(1350, 1180);

    int winnerGain = newWinnerElo - 1350;
    int loserLoss  = 1180 - newLoserElo;
    CHECK(winnerGain == loserLoss);
}
