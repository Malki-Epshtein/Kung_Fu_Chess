#include "../doctest.h"
#include "../server/app/logic/Matchmaker.h"
#include <memory>

namespace {
    Matchmaker::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }
}

TEST_CASE("Matchmaker - findMatch על מאגר ריק מחזיר nullopt") {
    Matchmaker mm;
    CHECK_FALSE(mm.findMatch(1200).has_value());
}

TEST_CASE("Matchmaker - שחקן בטווח 100 נמצא") {
    Matchmaker mm;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    mm.addToPool(hdlA, 1200);

    auto match = mm.findMatch(1290);
    REQUIRE(match.has_value());
}

TEST_CASE("Matchmaker - שחקן בדיוק בגבול 100 נמצא (inclusive)") {
    Matchmaker mm;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    mm.addToPool(hdlA, 1200);

    CHECK(mm.findMatch(1300).has_value());
    CHECK(mm.findMatch(1100).has_value());
}

TEST_CASE("Matchmaker - שחקן מחוץ לטווח 100 לא נמצא") {
    Matchmaker mm;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    mm.addToPool(hdlA, 1200);

    CHECK_FALSE(mm.findMatch(1301).has_value());
    CHECK_FALSE(mm.findMatch(1099).has_value());
}

TEST_CASE("Matchmaker - remove מוציא שחקן מהמאגר") {
    Matchmaker mm;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    mm.addToPool(hdlA, 1200);
    mm.remove(hdlA);

    CHECK_FALSE(mm.findMatch(1200).has_value());
}

TEST_CASE("Matchmaker - remove על שחקן שלא במאגר לא קורס") {
    Matchmaker mm;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    mm.remove(hdlA);
    CHECK_FALSE(mm.findMatch(1200).has_value());
}

TEST_CASE("Matchmaker - isWaiting מחזיר true לשחקן שנוסף למאגר") {
    Matchmaker mm;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    mm.addToPool(hdlA, 1200);

    CHECK(mm.isWaiting(hdlA));
}

TEST_CASE("Matchmaker - isWaiting מחזיר false לשחקן שהוסר") {
    Matchmaker mm;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    mm.addToPool(hdlA, 1200);
    mm.remove(hdlA);

    CHECK_FALSE(mm.isWaiting(hdlA));
}

TEST_CASE("Matchmaker - isWaiting מחזיר false לשחקן שלא נוסף מעולם") {
    Matchmaker mm;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    CHECK_FALSE(mm.isWaiting(hdlA));
}

TEST_CASE("Matchmaker - מוצא את השחקן הראשון המתאים מבין כמה") {
    Matchmaker mm;
    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    mm.addToPool(hdlA, 900);   // too far from 1200
    mm.addToPool(hdlB, 1250);  // within range

    auto match = mm.findMatch(1200);
    REQUIRE(match.has_value());
}
