#include "WsClient.h"
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>

namespace {
    using WsppClient = websocketpp::client<websocketpp::config::asio_client>;
}

void WsClient::run(const std::string& host, uint16_t port, const std::string& messageToSend) {
    WsppClient client;
    client.clear_access_channels(websocketpp::log::alevel::all);
    client.clear_error_channels(websocketpp::log::elevel::all);
    client.init_asio();

    client.set_open_handler([&client, &messageToSend](websocketpp::connection_hdl hdl) {
        std::cout << "[client] connected, sending: " << messageToSend << std::endl;
        websocketpp::lib::error_code ec;
        client.send(hdl, messageToSend, websocketpp::frame::opcode::text, ec);
        if (ec)
            std::cout << "[client] send failed: " << ec.message() << std::endl;
    });
    client.set_message_handler([](websocketpp::connection_hdl, WsppClient::message_ptr msg) {
        std::cout << "[client] received: " << msg->get_payload() << std::endl;
    });
    client.set_fail_handler([&client](websocketpp::connection_hdl hdl) {
        std::cout << "[client] connection failed: " << client.get_con_from_hdl(hdl)->get_ec().message() << std::endl;
    });
    client.set_close_handler([](websocketpp::connection_hdl) {
        std::cout << "[client] connection closed" << std::endl;
    });

    websocketpp::lib::error_code ec;
    std::string uri = "ws://" + host + ":" + std::to_string(port);
    WsppClient::connection_ptr con = client.get_connection(uri, ec);
    if (ec) {
        std::cout << "[client] could not create connection: " << ec.message() << std::endl;
        return;
    }

    client.connect(con);
    client.run();
}
