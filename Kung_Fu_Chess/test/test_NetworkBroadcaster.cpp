#include "../doctest.h"
#include "../server/app/NetworkBroadcaster.h"
#include "../server/app/GameSession.h"

TEST_CASE("NetworkBroadcaster - שולח את ה-snapshot המפורסם דרך ה-sender שלו") {
    EventBus bus;
    std::string sent;
    int sendCount = 0;

    NetworkBroadcaster broadcaster(bus, GameSession::snapshotTopic("room-a"), [&](const std::string& text) {
        sent = text;
        sendCount++;
    });

    bus.publish(GameSession::snapshotTopic("room-a"), { {"board_width", 8} });

    CHECK(sendCount == 1);
    CHECK(sent.find("\"board_width\":8") != std::string::npos);
}

TEST_CASE("NetworkBroadcaster - לא שולח כלום על נושאים אחרים") {
    EventBus bus;
    int sendCount = 0;

    NetworkBroadcaster broadcaster(bus, GameSession::snapshotTopic("room-a"), [&](const std::string&) { sendCount++; });

    bus.publish("some-other-topic", {});

    CHECK(sendCount == 0);
}

TEST_CASE("NetworkBroadcaster - broadcaster של חדר אחד לא מקבל שידורים של חדר אחר") {
    EventBus bus;
    int roomASends = 0;
    int roomBSends = 0;

    NetworkBroadcaster broadcasterA(bus, GameSession::snapshotTopic("room-a"), [&](const std::string&) { roomASends++; });
    NetworkBroadcaster broadcasterB(bus, GameSession::snapshotTopic("room-b"), [&](const std::string&) { roomBSends++; });

    bus.publish(GameSession::snapshotTopic("room-a"), { {"board_width", 8} });

    CHECK(roomASends == 1);
    CHECK(roomBSends == 0);
}
