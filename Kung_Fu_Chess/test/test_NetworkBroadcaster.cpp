#include "../doctest.h"
#include "../server/app/NetworkBroadcaster.h"
#include "../server/app/GameSession.h"

TEST_CASE("NetworkBroadcaster - שולח את ה-snapshot המפורסם דרך ה-sender שלו") {
    EventBus bus;
    std::string sent;
    int sendCount = 0;

    NetworkBroadcaster broadcaster(bus, [&](const std::string& text) {
        sent = text;
        sendCount++;
    });

    bus.publish(GameSession::kSnapshotTopic, { {"board_width", 8} });

    CHECK(sendCount == 1);
    CHECK(sent.find("\"board_width\":8") != std::string::npos);
}

TEST_CASE("NetworkBroadcaster - לא שולח כלום על נושאים אחרים") {
    EventBus bus;
    int sendCount = 0;

    NetworkBroadcaster broadcaster(bus, [&](const std::string&) { sendCount++; });

    bus.publish("some-other-topic", {});

    CHECK(sendCount == 0);
}
