#include "../doctest.h"
#include "../shared/protocol/MessageCodec.h"

TEST_CASE("MessageCodec - קידוד ופענוח של הודעת MOVE משמרים את הסוג והתוכן") {
    Message original;
    original.type = MessageType::Move;
    original.payload = { {"from", "e2"}, {"to", "e4"} };

    std::string encoded = MessageCodec::encode(original);
    Message decoded = MessageCodec::decode(encoded);

    CHECK(decoded.type == MessageType::Move);
    CHECK(decoded.payload.at("from") == "e2");
    CHECK(decoded.payload.at("to") == "e4");
}

TEST_CASE("MessageCodec - הודעת HELLO ללא payload מפוענחת עם payload ריק") {
    Message original;
    original.type = MessageType::Hello;

    Message decoded = MessageCodec::decode(MessageCodec::encode(original));

    CHECK(decoded.type == MessageType::Hello);
    CHECK(decoded.payload.empty());
}

TEST_CASE("MessageCodec - כל סוגי ההודעות עוברים round-trip") {
    for (MessageType type : { MessageType::Hello, MessageType::Move, MessageType::Jump, MessageType::Snapshot, MessageType::Login }) {
        Message original;
        original.type = type;
        Message decoded = MessageCodec::decode(MessageCodec::encode(original));
        CHECK(decoded.type == type);
    }
}

TEST_CASE("MessageCodec - טקסט לא תקין זורק חריגה") {
    CHECK_THROWS(MessageCodec::decode("not json"));
}

TEST_CASE("MessageCodec - סוג הודעה לא מוכר זורק חריגה") {
    CHECK_THROWS(MessageCodec::decode(R"({"type":"NOT_A_REAL_TYPE","payload":{}})"));
}
