#include "../doctest.h"
#include "../server/app/handlers/MessageDispatcher.h"
#include "../server/app/networking/BroadcasterManager.h"
#include "../server/app/session/SessionRegistry.h"
#include "../server/app/session/ClientSessionRegistry.h"
#include "../server/app/logic/Matchmaker.h"
#include "../server/app/logic/EloService.h"
#include "../server/db/UserRepository.h"
#include "../shared/protocol/MessageCodec.h"
#include <asio/io_context.hpp>
#include <memory>

namespace {
    MessageDispatcher::ConnectionHandle handleFrom(std::shared_ptr<int>& keepAlive) {
        keepAlive = std::make_shared<int>();
        return keepAlive;
    }

    // Bundles every MessageDispatcher dependency, same shape as WsServer's
    // own composition - lets each TEST_CASE build one with a single call.
    struct Fixture {
        UserRepository          users{ ":memory:" };
        ClientSessionRegistry  clientSessions;
        SessionRegistry          registry;
        EventBus                  bus;
        Matchmaker                matchmaker;
        EloService                eloService{ users };
        asio::io_context          io;
        BroadcasterManager        broadcasters{ bus, registry, [](MessageDispatcher::ConnectionHandle, const std::string&) {} };
        MessageDispatcher          dispatcher{ users, clientSessions, registry, bus, matchmaker,
                                                broadcasters, eloService, [](MessageDispatcher::ConnectionHandle, const std::string&) {}, io };
    };

    nlohmann::json send(Fixture& f, MessageDispatcher::ConnectionHandle hdl, MessageType type, const nlohmann::json& payload = {}) {
        std::string reply = f.dispatcher.process(hdl, MessageCodec::encode(Message{ type, payload }));
        return nlohmann::json::parse(reply);
    }
}

TEST_CASE("MessageDispatcher - LOGIN מנותב ל-LoginHandler ומצליח") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);

    nlohmann::json reply = send(f, hdlA, MessageType::Login, { {"username", "alice"}, {"password", "pw"} });

    CHECK(reply.at("success").get<bool>());
    CHECK(reply.at("elo").get<int>() == 1200);
}

TEST_CASE("MessageDispatcher - CREATE_ROOM מנותב ל-CreateRoomHandler; שם תפוס נכשל") {
    Fixture f;
    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);

    nlohmann::json first = send(f, hdlA, MessageType::CreateRoom, { {"name", "room-a"} });
    CHECK(first.at("success").get<bool>());
    CHECK(first.at("role").get<std::string>() == "White");

    nlohmann::json second = send(f, hdlB, MessageType::CreateRoom, { {"name", "room-a"} });
    CHECK_FALSE(second.at("success").get<bool>());
}

TEST_CASE("MessageDispatcher - JOIN_ROOM מנותב ל-JoinRoomHandler; חדר לא קיים נכשל") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);

    nlohmann::json reply = send(f, hdlA, MessageType::JoinRoom, { {"name", "no-such-room"} });
    CHECK_FALSE(reply.at("success").get<bool>());
}

TEST_CASE("MessageDispatcher - FIND_GAME מנותב ל-FindGameHandler ומשדך שני שחקנים") {
    Fixture f;
    std::shared_ptr<int> a, b;
    auto hdlA = handleFrom(a);
    auto hdlB = handleFrom(b);
    send(f, hdlA, MessageType::Login, { {"username", "alice"}, {"password", "pw"} });
    send(f, hdlB, MessageType::Login, { {"username", "bob"}, {"password", "pw"} });

    nlohmann::json waiting = send(f, hdlA, MessageType::FindGame);
    CHECK(waiting.at("message").get<std::string>() == "searching for opponent");
    CHECK_FALSE(waiting.contains("type")); // plain ack, not a GAME_FOUND envelope

    nlohmann::json matched = send(f, hdlB, MessageType::FindGame);
    CHECK(matched.at("type").get<std::string>() == "GAME_FOUND");
    CHECK(matched.at("payload").at("role").get<std::string>() == "Black");
}

TEST_CASE("MessageDispatcher - MOVE ללא handler רשום נופל ל-CommandDispatcher ומצליח על הלוח האמיתי") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);
    send(f, hdlA, MessageType::CreateRoom, { {"name", "room-a"} }); // creator -> White, real starting board

    nlohmann::json reply = send(f, hdlA, MessageType::Move,
        { {"from", {{"row", 6}, {"col", 0}}}, {"to", {{"row", 5}, {"col", 0}}} });

    CHECK(reply.at("success").get<bool>());
    CHECK(reply.at("role").get<std::string>() == "White");
}

TEST_CASE("MessageDispatcher - MOVE מחיבור שלא נמצא בשום חדר נכשל בלי לקרוס") {
    Fixture f;
    std::shared_ptr<int> a;
    auto hdlA = handleFrom(a);

    nlohmann::json reply = send(f, hdlA, MessageType::Move,
        { {"from", {{"row", 6}, {"col", 0}}}, {"to", {{"row", 5}, {"col", 0}}} });

    CHECK_FALSE(reply.at("success").get<bool>());
}
