#include "../doctest.h"
#include "../server/db/UserRepository.h"

TEST_CASE("UserRepository - שם משתמש חדש נוצר עם ELO התחלתי 1200") {
    UserRepository repo(":memory:");
    LoginResult result = repo.login("malki", "secret");

    CHECK(result.success);
    CHECK(result.elo == 1200);
}

TEST_CASE("UserRepository - כניסה חוזרת עם הסיסמה הנכונה מצליחה ומחזירה את אותו ELO") {
    UserRepository repo(":memory:");
    repo.login("malki", "secret");

    LoginResult result = repo.login("malki", "secret");

    CHECK(result.success);
    CHECK(result.elo == 1200);
}

TEST_CASE("UserRepository - כניסה עם סיסמה שגויה נכשלת") {
    UserRepository repo(":memory:");
    repo.login("malki", "secret");

    LoginResult result = repo.login("malki", "wrong-password");

    CHECK_FALSE(result.success);
}

TEST_CASE("UserRepository - שני שמות משתמש שונים לא מתנגשים") {
    UserRepository repo(":memory:");
    repo.login("malki", "secret1");
    repo.login("shira", "secret2");

    CHECK(repo.login("malki", "secret1").success);
    CHECK(repo.login("shira", "secret2").success);
    CHECK_FALSE(repo.login("malki", "secret2").success);
}

TEST_CASE("UserRepository - שני מופעים נפרדים של DB בזיכרון לא חולקים נתונים") {
    UserRepository repoA(":memory:");
    UserRepository repoB(":memory:");

    repoA.login("malki", "secret");

    // repoB has never seen "malki" before, so this is a fresh account there.
    LoginResult result = repoB.login("malki", "different-secret");
    CHECK(result.success);
    CHECK(result.message == "account created");
}
