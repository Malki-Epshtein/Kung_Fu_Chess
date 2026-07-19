#include "WsServer.h"
#include "../../shared/protocol/MessageCodec.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>

namespace {
    using WsppServer = websocketpp::server<websocketpp::config::asio>;

    const char* typeName(MessageType type) {
        switch (type) {
            case MessageType::Hello:    return "HELLO";
            case MessageType::Move:     return "MOVE";
            case MessageType::Jump:     return "JUMP";
            case MessageType::Snapshot: return "SNAPSHOT";
        }
        return "UNKNOWN";
    }
}

void WsServer::run(uint16_t port) {
    WsppServer server;
    server.clear_access_channels(websocketpp::log::alevel::all);
    server.clear_error_channels(websocketpp::log::elevel::all);
    server.init_asio();

    server.set_open_handler([](websocketpp::connection_hdl) {
        std::cout << "[server] client connected" << std::endl;
    });
    server.set_close_handler([](websocketpp::connection_hdl) {
        std::cout << "[server] client disconnected" << std::endl;
    });
    server.set_message_handler([&server](websocketpp::connection_hdl hdl, WsppServer::message_ptr msg) {
        const std::string& text = msg->get_payload();
        try {
            Message decoded = MessageCodec::decode(text);
            std::cout << "[server] received " << typeName(decoded.type) << ": " << text << std::endl;
        } catch (const std::exception& e) {
            std::cout << "[server] received non-protocol text: " << text
                       << " (decode failed: " << e.what() << ")" << std::endl;
        }

        server.send(hdl, text, msg->get_opcode());
        std::cout << "[server] echoed back" << std::endl;
    });

    server.listen(port);
    server.start_accept();
    std::cout << "[server] listening on port " << port << std::endl;
    server.run();
}
