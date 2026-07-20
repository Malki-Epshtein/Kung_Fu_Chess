#include "../doctest.h"
#include "../server/app/GameSession.h"
#include "../shared/model/Board.h"
#include <memory>

TEST_CASE("GameSession - tick מפרסם snapshot לנושא הנכון על ה-bus") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus);

    nlohmann::json received;
    bool called = false;
    bus.subscribe(GameSession::kSnapshotTopic, [&](const nlohmann::json& data) {
        called = true;
        received = data;
    });

    session.tick(30);

    CHECK(called);
    CHECK(received.at("board_width") == 8);
    CHECK(received.at("board_height") == 8);
}

TEST_CASE("GameSession - חושף את ה-engine שלה לשימוש חיצוני (למשל CommandDispatcher)") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus);

    CHECK_FALSE(session.engine().isGameOver());
}

// ==================== Stage D: disconnect status ====================

TEST_CASE("GameSession - ללא ניתוק פעיל, השידור לא מכיל מפתח disconnect") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus);

    nlohmann::json received;
    bus.subscribe(GameSession::kSnapshotTopic, [&](const nlohmann::json& data) { received = data; });

    session.tick(30);

    CHECK_FALSE(received.contains("disconnect"));
}

TEST_CASE("GameSession - ניתוק פעיל מתווסף לשידור הרגיל") {
    auto board = std::make_shared<Board>(8, 8);
    EventBus bus;
    GameSession session(board, bus);
    session.setDisconnectStatus({ true, Chess::Color::White, 17 });

    nlohmann::json received;
    bus.subscribe(GameSession::kSnapshotTopic, [&](const nlohmann::json& data) { received = data; });

    session.tick(30);

    REQUIRE(received.contains("disconnect"));
    CHECK(received.at("disconnect").at("active") == true);
    CHECK(received.at("disconnect").at("color") == "White");
    CHECK(received.at("disconnect").at("secondsRemaining") == 17);
}
