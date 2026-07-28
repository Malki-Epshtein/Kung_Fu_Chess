#include "ApiGateway.h"
#include "../shared/log/Log.h"
#include "json.hpp"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {
    using HttpServer = websocketpp::server<websocketpp::config::asio>;
    using Relay       = websocketpp::client<websocketpp::config::asio_client>;
    using ConnectionHandle = websocketpp::connection_hdl;

    // Not cryptographically secure - same "student project, not production
    // auth" reasoning as UserRepository's password hash. Good enough that a
    // token isn't trivially guessable in a local/demo deployment.
    std::string makeToken() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dist;
        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(16) << dist(gen) << std::setw(16) << dist(gen);
        return oss.str();
    }

    // State for one in-flight shard relay (a POST /login or POST /rooms
    // call waiting on a shard's reply over a throwaway internal WS
    // connection). One HTTP request in flight per relay - keyed by the
    // relay connection's own handle, not the original HTTP connection's.
    struct RelayState {
        std::string outgoing; // the encoded Message to send once open
        std::function<void(bool ok, const nlohmann::json& reply)> callback;
        bool done = false; // guards against firing callback twice (message then close)
    };
    using RelayPtr = std::shared_ptr<RelayState>;
    using RelayMap = std::map<ConnectionHandle, RelayPtr, std::owner_less<ConnectionHandle>>;

    // Splits "POST /rooms/ROOMNAME/join" style paths - returns "" if `uri`
    // doesn't match "/rooms/<name>/join" exactly (used to distinguish that
    // route from plain POST /rooms).
    std::string extractJoinRoomName(const std::string& uri) {
        const std::string prefix = "/rooms/";
        const std::string suffix = "/join";
        if (uri.size() <= prefix.size() + suffix.size()) return "";
        if (uri.compare(0, prefix.size(), prefix) != 0) return "";
        if (uri.compare(uri.size() - suffix.size(), suffix.size(), suffix) != 0) return "";
        return uri.substr(prefix.size(), uri.size() - prefix.size() - suffix.size());
    }

    // Both take a connection_ptr directly rather than (HttpServer&,
    // ConnectionHandle) - deliberately NOT re-resolved from a
    // connection_hdl at the point of responding. A connection_hdl is a
    // weak_ptr: it does not keep the connection alive by itself. The two
    // deferred call sites below (relayToShard's callbacks) used to capture
    // only `hdl`, and the connection object was owned solely by the http
    // handler's own local `con` - once that handler returned (right after
    // defer_http_response()), `con` went out of scope and, with nothing
    // else holding a strong reference during the async wait, the
    // connection was destroyed via plain refcounting. No close/fail event
    // ever fired (confirmed with temporary handlers) because nothing
    // network-level actually happened - hdl.lock() just silently started
    // returning null, sometimes within milliseconds. Capturing the
    // connection_ptr itself (as done below) keeps it alive for exactly as
    // long as the callback holding it exists.

    // For a handler that answers inline (the normal case - everything here
    // except the two relayToShard callbacks below) - just sets status/body
    // and returns; the caller must `return` from the http handler right
    // after this and let websocketpp send the response itself once the
    // handler returns. Calling send_http_response() (see respondDeferred
    // below) is only legal once defer_http_response() was called first -
    // otherwise it throws ("invalid state", websocketpp's
    // connection_impl.hpp:689) since the connection was never put into the
    // deferred http_state that method requires.
    void respondSync(const HttpServer::connection_ptr& con, int status, const nlohmann::json& body) {
        con->append_header("Content-Type", "application/json");
        con->set_status(static_cast<websocketpp::http::status_code::value>(status));
        con->set_body(body.dump());
    }

    // For a connection that already called con->defer_http_response() -
    // its http handler already returned once without answering, so
    // websocketpp is waiting for an explicit send_http_response() call,
    // made later from here (the relayToShard callback, once the shard's
    // reply actually arrives).
    void respondDeferred(const HttpServer::connection_ptr& con, int status, const nlohmann::json& body) {
        con->append_header("Content-Type", "application/json");
        con->set_status(static_cast<websocketpp::http::status_code::value>(status));
        con->set_body(body.dump());
        con->send_http_response();
    }
}

void ApiGateway::run(asio::io_context& io, uint16_t listenPort, const std::vector<std::string>& shardUris,
                      IRoomDirectory& directory, IClientSessionStore& sessionStore) {
    // Heap-allocated and intentionally never freed - io.run() (called by
    // gateway_main.cpp/api_gateway_main.cpp, after this function returns)
    // needs all of these to outlive this function's return: run() only
    // installs handlers and starts listening, it never calls io.run()
    // itself. relays/roundRobinNext used to be plain stack locals here,
    // captured by reference into the handlers below - since those handlers
    // are what io.run() actually invokes later, every one of them was a
    // dangling reference the instant run() returned, corrupting memory on
    // the very first real request (crashed with a different symptom each
    // time - a pthread assertion once, "invalid state" another time -
    // classic undefined-behavior fingerprint, not a deterministic bug).
    HttpServer& http  = *new HttpServer();
    Relay&      relay = *new Relay();
    http.clear_access_channels(websocketpp::log::alevel::all);
    http.clear_error_channels(websocketpp::log::elevel::all);
    relay.clear_access_channels(websocketpp::log::alevel::all);
    relay.clear_error_channels(websocketpp::log::elevel::all);

    http.init_asio(&io);
    relay.init_asio(&io);

    RelayMap& relays = *new RelayMap();
    size_t&   roundRobinNext = *new size_t(0);

    // Opens a throwaway WS connection to `uri`, sends the encoded message,
    // and calls `callback(ok, reply)` once a reply arrives (or the
    // connection fails/closes before one does). This is how POST /login
    // and POST /rooms reuse LoginHandler/CreateRoomHandler unchanged - the
    // Gateway is just another client from the shard's point of view.
    //
    // Heap-allocated (new auto(...), same reasoning as http/relay above) -
    // unlike onRelayGone below, this is never passed BY VALUE to a
    // set_xxx_handler() call that would make its own independent copy; it's
    // called by name from inside the http handler lambda, which only
    // captures a REFERENCE to it. A stack-local here would leave that
    // reference dangling the instant run() returns, same bug as
    // relays/roundRobinNext had.
    auto& relayToShard = *new auto([&](const std::string& uri, const std::string& typeName, const nlohmann::json& payload,
                             std::function<void(bool, const nlohmann::json&)> callback) {
        nlohmann::json envelope = { {"type", typeName}, {"payload", payload} };

        websocketpp::lib::error_code ec;
        Relay::connection_ptr con = relay.get_connection(uri, ec);
        if (ec) {
            callback(false, {});
            return;
        }
        auto state = std::make_shared<RelayState>();
        state->outgoing = envelope.dump();
        state->callback = std::move(callback);
        relays[con->get_handle()] = state;
        relay.connect(con);
    });

    relay.set_open_handler([&](ConnectionHandle hdl) {
        auto it = relays.find(hdl);
        if (it == relays.end()) return;
        websocketpp::lib::error_code ec;
        relay.send(hdl, it->second->outgoing, websocketpp::frame::opcode::text, ec);
        if (ec) spdlog::error("api gateway: relay send failed: {}", ec.message());
    });

    relay.set_message_handler([&](ConnectionHandle hdl, Relay::message_ptr msg) {
        auto it = relays.find(hdl);
        if (it == relays.end()) return;
        RelayPtr state = it->second;
        state->done = true;

        nlohmann::json reply;
        bool ok = true;
        try {
            reply = nlohmann::json::parse(msg->get_payload());
        } catch (const std::exception&) {
            ok = false;
        }
        state->callback(ok, reply);

        websocketpp::lib::error_code ec;
        relay.close(hdl, websocketpp::close::status::normal, "relay complete", ec);
        relays.erase(it);
    });

    auto onRelayGone = [&](ConnectionHandle hdl) {
        auto it = relays.find(hdl);
        if (it == relays.end()) return;
        if (!it->second->done)
            it->second->callback(false, {});
        relays.erase(it);
    };
    relay.set_close_handler(onRelayGone);
    relay.set_fail_handler(onRelayGone);

    http.set_http_handler([&](ConnectionHandle hdl) {
        auto con = http.get_con_from_hdl(hdl);
        const std::string method = con->get_request().get_method();
        const std::string uri    = con->get_request().get_uri();
        const std::string body   = con->get_request_body();

        nlohmann::json request;
        try {
            if (!body.empty()) request = nlohmann::json::parse(body);
        } catch (const std::exception& e) {
            respondSync(con, 400, { {"success", false}, {"message", std::string("malformed body: ") + e.what()} });
            return;
        }

        if (method == "POST" && uri == "/login") {
            std::string username = request.value("username", "");
            std::string password = request.value("password", "");
            const std::string& shardUri = shardUris[roundRobinNext % shardUris.size()];
            ++roundRobinNext;

            con->defer_http_response();
            // con captured BY VALUE - keeps the connection alive for as
            // long as this callback exists (see respondDeferred's comment
            // above). hdl (a weak_ptr) would not have done that.
            relayToShard(shardUri, "LOGIN", { {"username", username}, {"password", password} },
                [con, &sessionStore, username](bool ok, const nlohmann::json& reply) {
                    if (!ok) {
                        respondDeferred(con, 502, { {"success", false}, {"message", "upstream unavailable"} });
                        return;
                    }
                    if (!reply.value("success", false)) {
                        respondDeferred(con, 401, reply);
                        return;
                    }
                    std::string token = makeToken();
                    sessionStore.put(token, username, reply.value("elo", 0));
                    respondDeferred(con, 200, { {"success", true}, {"message", reply.value("message", "")},
                                              {"elo", reply.value("elo", 0)}, {"token", token} });
                });
            return;
        }

        if (method == "POST" && uri == "/rooms") {
            std::string token = request.value("token", "");
            std::string name  = request.value("name", "");
            if (!sessionStore.get(token)) {
                respondSync(con, 401, { {"success", false}, {"message", "invalid or expired token"} });
                return;
            }
            const std::string& shardUri = shardUris[roundRobinNext % shardUris.size()];
            ++roundRobinNext;

            con->defer_http_response();
            // autoJoin:false - this is a throwaway connection, about to
            // close; the real client claims the seat later, over its own
            // WebSocket, via EnterRoom (see CreateRoomHandler.cpp).
            relayToShard(shardUri, "CREATE_ROOM", { {"name", name}, {"autoJoin", false} },
                [con, shardUri](bool ok, const nlohmann::json& reply) {
                    if (!ok) {
                        respondDeferred(con, 502, { {"success", false}, {"message", "upstream unavailable"} });
                        return;
                    }
                    if (!reply.value("success", false)) {
                        respondDeferred(con, 409, reply);
                        return;
                    }
                    respondDeferred(con, 200, { {"success", true}, {"message", reply.value("message", "")},
                                              {"roomName", reply.value("roomName", "")}, {"shard", shardUri} });
                });
            return;
        }

        std::string joinRoomName = extractJoinRoomName(uri);
        if (method == "POST" && !joinRoomName.empty()) {
            std::string token = request.value("token", "");
            if (!sessionStore.get(token)) {
                respondSync(con, 401, { {"success", false}, {"message", "invalid or expired token"} });
                return;
            }
            // No relay needed - the room (and its shard) already exists;
            // this is a plain directory lookup, resolved synchronously.
            std::string shardUri = directory.get(joinRoomName);
            if (shardUri.empty()) {
                respondSync(con, 404, { {"success", false}, {"message", "room not found"} });
                return;
            }
            respondSync(con, 200, { {"success", true}, {"message", "room found"},
                                      {"roomName", joinRoomName}, {"shard", shardUri} });
            return;
        }

        if (method == "GET" && uri == "/rooms") {
            respondSync(con, 200, { {"success", true}, {"rooms", directory.list()} });
            return;
        }

        if (method == "GET" && uri == "/history") {
            // Stub - real match history needs a games/results table in
            // Postgres that doesn't exist yet (see the plan's "explicitly
            // out of scope").
            respondSync(con, 200, { {"success", true}, {"games", nlohmann::json::array()} });
            return;
        }

        respondSync(con, 404, { {"success", false}, {"message", "no such route"} });
    });

    http.listen(listenPort);
    http.start_accept();
    spdlog::info("gateway: API listening on port {}", listenPort);

    // No io.run() here - gateway_main.cpp calls it once, after WsGateway
    // has set up its own listener on the same io_context.
}
