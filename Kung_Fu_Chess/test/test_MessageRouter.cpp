#include "../doctest.h"
#include "../server/app/handlers/MessageRouter.h"

namespace {
    class FakeHandler : public IMessageHandler {
    public:
        nlohmann::json handle(ConnectionHandle, const nlohmann::json&) override {
            return { {"handled", true} };
        }
    };
}

TEST_CASE("MessageRouter - find מחזיר nullptr לטיפוס שלא נרשם") {
    MessageRouter router;
    CHECK(router.find(MessageType::Login) == nullptr);
}

TEST_CASE("MessageRouter - find מחזיר את ה-handler שנרשם לטיפוס") {
    MessageRouter router;
    FakeHandler handler;
    router.registerHandler(MessageType::Login, handler);

    CHECK(router.find(MessageType::Login) == &handler);
}

TEST_CASE("MessageRouter - טיפוסים שונים לא מתערבבים") {
    MessageRouter router;
    FakeHandler loginHandler;
    FakeHandler roomHandler;
    router.registerHandler(MessageType::Login, loginHandler);
    router.registerHandler(MessageType::CreateRoom, roomHandler);

    CHECK(router.find(MessageType::Login) == &loginHandler);
    CHECK(router.find(MessageType::CreateRoom) == &roomHandler);
    CHECK(router.find(MessageType::JoinRoom) == nullptr);
}
