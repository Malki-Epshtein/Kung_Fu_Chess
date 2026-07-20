#include "../doctest.h"
#include "../shared/bus/EventBus.h"

TEST_CASE("EventBus - מנוי מקבל את הנתונים שפורסמו לנושא שלו") {
    EventBus bus;
    nlohmann::json received;
    bool called = false;

    bus.subscribe("topic-a", [&](const nlohmann::json& data) {
        called = true;
        received = data;
    });

    bus.publish("topic-a", { {"value", 42} });

    CHECK(called);
    CHECK(received.at("value") == 42);
}

TEST_CASE("EventBus - כמה מנויים על אותו נושא מקבלים את אותה הודעה") {
    EventBus bus;
    int callCount = 0;

    bus.subscribe("topic-a", [&](const nlohmann::json&) { callCount++; });
    bus.subscribe("topic-a", [&](const nlohmann::json&) { callCount++; });

    bus.publish("topic-a", {});

    CHECK(callCount == 2);
}

TEST_CASE("EventBus - פרסום לנושא בלי מנויים לא מקריס") {
    EventBus bus;
    CHECK_NOTHROW(bus.publish("no-subscribers", { {"x", 1} }));
}

TEST_CASE("EventBus - מנוי לנושא אחד לא מופעל מפרסום לנושא אחר") {
    EventBus bus;
    bool called = false;

    bus.subscribe("topic-a", [&](const nlohmann::json&) { called = true; });
    bus.publish("topic-b", {});

    CHECK_FALSE(called);
}
