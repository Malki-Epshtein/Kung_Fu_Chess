#include "../doctest.h"
#include "../server/app/logic/GameStateMirrorService.h"
#include "../server/app/session/SessionRegistry.h"
#include "../server/app/session/GameSession.h"
#include "../shared/db/IGameStateStore.h"
#include "../shared/model/Board.h"
#include <map>
#include <memory>

namespace {
    std::shared_ptr<Board> makeBoard() { return std::make_shared<Board>(8, 8); }

    // Records every save()/clear() call instead of touching real Redis -
    // same "fake instead of a live connection" idiom this codebase already
    // uses for its other optional Redis-backed dependencies in tests.
    class FakeGameStateStore : public IGameStateStore {
    public:
        struct SavedState {
            std::string board;
            std::string moveLog;
            std::string whiteUsername;
            int         whiteElo = 0;
            std::string blackUsername;
            int         blackElo = 0;
        };

        void save(const std::string& roomName, const std::string& boardStateJson,
                  const std::string& moveLogJson, const std::string& whiteUsername, int whiteElo,
                  const std::string& blackUsername, int blackElo) override {
            ++saveCount[roomName];
            saved[roomName] = { boardStateJson, moveLogJson, whiteUsername, whiteElo, blackUsername, blackElo };
        }

        void clear(const std::string& roomName) override {
            ++clearCount[roomName];
            saved.erase(roomName);
        }

        std::map<std::string, int> saveCount;
        std::map<std::string, int> clearCount;
        std::map<std::string, SavedState> saved;
    };
}

TEST_CASE("GameStateMirrorService - moveLogTopic מפעיל save עם board ו-moveLog לא ריקים") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);

    FakeGameStateStore store;
    GameStateMirrorService mirror(&store, registry);
    mirror.attach(bus, "room-a");

    bus.publish(GameSession::moveLogTopic("room-a"), {});

    REQUIRE(store.saveCount["room-a"] == 1);
    CHECK_FALSE(store.saved["room-a"].board.empty());
    CHECK_FALSE(store.saved["room-a"].moveLog.empty());
}

TEST_CASE("GameStateMirrorService - gameEndedTopic מפעיל clear") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);

    FakeGameStateStore store;
    GameStateMirrorService mirror(&store, registry);
    mirror.attach(bus, "room-a");

    bus.publish(GameSession::moveLogTopic("room-a"), {});
    REQUIRE(store.saved.count("room-a") == 1);

    bus.publish(GameSession::gameEndedTopic("room-a"), {});

    CHECK(store.clearCount["room-a"] == 1);
    CHECK(store.saved.count("room-a") == 0);
}

TEST_CASE("GameStateMirrorService - store null לא קורס ולא עושה כלום") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);

    GameStateMirrorService mirror(nullptr, registry);
    mirror.attach(bus, "room-a");

    bus.publish(GameSession::moveLogTopic("room-a"), {});
    bus.publish(GameSession::gameEndedTopic("room-a"), {});
    // No CHECK needed beyond "this didn't crash" - there's nothing to
    // observe with a null store.
}

TEST_CASE("GameStateMirrorService - חדרים שונים מנוהלים בנפרד") {
    SessionRegistry registry;
    EventBus bus;
    registry.createRoom("room-a", makeBoard(), bus);
    registry.createRoom("room-b", makeBoard(), bus);

    FakeGameStateStore store;
    GameStateMirrorService mirror(&store, registry);
    mirror.attach(bus, "room-a");
    mirror.attach(bus, "room-b");

    bus.publish(GameSession::moveLogTopic("room-a"), {});

    CHECK(store.saveCount["room-a"] == 1);
    CHECK(store.saveCount["room-b"] == 0);
}
