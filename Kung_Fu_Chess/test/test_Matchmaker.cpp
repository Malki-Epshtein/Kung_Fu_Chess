#include "../doctest.h"
#include "../shared/matchmaking/Matchmaker.h"

TEST_CASE("Matchmaker - findMatch על מאגר ריק מחזיר nullopt") {
    Matchmaker mm;
    CHECK_FALSE(mm.findMatch(1200).has_value());
}

TEST_CASE("Matchmaker - שחקן בטווח 100 נמצא") {
    Matchmaker mm;
    mm.addToPool("ticket-a", 1200);

    auto match = mm.findMatch(1290);
    REQUIRE(match.has_value());
    CHECK(*match == "ticket-a");
}

TEST_CASE("Matchmaker - שחקן בדיוק בגבול 100 נמצא (inclusive)") {
    Matchmaker mm;
    mm.addToPool("ticket-a", 1200);

    CHECK(mm.findMatch(1300).has_value());
    CHECK(mm.findMatch(1100).has_value());
}

TEST_CASE("Matchmaker - שחקן מחוץ לטווח 100 לא נמצא") {
    Matchmaker mm;
    mm.addToPool("ticket-a", 1200);

    CHECK_FALSE(mm.findMatch(1301).has_value());
    CHECK_FALSE(mm.findMatch(1099).has_value());
}

TEST_CASE("Matchmaker - remove מוציא שחקן מהמאגר") {
    Matchmaker mm;
    mm.addToPool("ticket-a", 1200);
    mm.remove("ticket-a");

    CHECK_FALSE(mm.findMatch(1200).has_value());
}

TEST_CASE("Matchmaker - remove על שחקן שלא במאגר לא קורס") {
    Matchmaker mm;
    mm.remove("ticket-a");
    CHECK_FALSE(mm.findMatch(1200).has_value());
}

TEST_CASE("Matchmaker - isWaiting מחזיר true לשחקן שנוסף למאגר") {
    Matchmaker mm;
    mm.addToPool("ticket-a", 1200);

    CHECK(mm.isWaiting("ticket-a"));
}

TEST_CASE("Matchmaker - isWaiting מחזיר false לשחקן שהוסר") {
    Matchmaker mm;
    mm.addToPool("ticket-a", 1200);
    mm.remove("ticket-a");

    CHECK_FALSE(mm.isWaiting("ticket-a"));
}

TEST_CASE("Matchmaker - isWaiting מחזיר false לשחקן שלא נוסף מעולם") {
    Matchmaker mm;
    CHECK_FALSE(mm.isWaiting("ticket-a"));
}

TEST_CASE("Matchmaker - מוצא את השחקן הראשון המתאים מבין כמה") {
    Matchmaker mm;
    mm.addToPool("ticket-a", 900);   // too far from 1200
    mm.addToPool("ticket-b", 1250);  // within range

    auto match = mm.findMatch(1200);
    REQUIRE(match.has_value());
    CHECK(*match == "ticket-b");
}
