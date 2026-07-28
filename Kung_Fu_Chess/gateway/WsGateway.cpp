#include "WsGateway.h"
#include "../shared/log/Log.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <algorithm>
#include <map>
#include <memory>
#include <vector>
#include <string>

namespace {
    using Downstream = websocketpp::server<websocketpp::config::asio>;
    using Upstream    = websocketpp::client<websocketpp::config::asio_client>;
    using ConnectionHandle = websocketpp::connection_hdl;

    // One per real client. downstreamHdl is the real client's connection to
    // this Gateway; upstreamHdl is this Gateway's own connection to the
    // shard standing in for that client. upstreamOpen/pending exist because
    // the upstream connect is asynchronous - a client can send a message
    // before that socket finishes opening, so the first message(s) are
    // queued rather than dropped.
    struct Link {
        ConnectionHandle    downstreamHdl;
        ConnectionHandle    upstreamHdl;
        bool                upstreamOpen = false;
        std::vector<std::string> pending;
    };
    using LinkPtr = std::shared_ptr<Link>;
    using LinkMap = std::map<ConnectionHandle, LinkPtr, std::owner_less<ConnectionHandle>>;
}

void WsGateway::run(asio::io_context& io, uint16_t listenPort, const std::vector<std::string>& shardUris) {
    // Heap-allocated and intentionally never freed: io.run() (called by
    // gateway_main.cpp, after this function returns) needs both endpoints
    // to stay alive for the rest of the process's lifetime, which a stack
    // local here can't do once run() itself returns before io.run() starts.
    // Same reasoning applies to byDownstream/byUpstream/roundRobinNext
    // below - they used to be plain stack locals, captured by reference
    // into the handlers just below them, which io.run() only invokes AFTER
    // this function has already returned (see the "No io.run() here"
    // comment at the bottom). Every one of those handlers held a dangling
    // reference the instant run() returned - undefined behavior on the
    // very first real client connection (this crashed hard, no log output
    // at all, unlike ApiGateway's equivalent bug which at least threw a
    // catchable exception - see ApiGateway.cpp's run() for the full
    // writeup of the same mistake, made there too).
    Downstream& downstream = *new Downstream();
    Upstream&   upstream   = *new Upstream();
    downstream.clear_access_channels(websocketpp::log::alevel::all);
    downstream.clear_error_channels(websocketpp::log::elevel::all);
    upstream.clear_access_channels(websocketpp::log::alevel::all);
    upstream.clear_error_channels(websocketpp::log::elevel::all);

    // Single-threaded, same guarantee WsServer relies on: no handler ever
    // runs concurrently with another, so the two LinkMaps below (and
    // roundRobinNext) need no locking. `io` is owned by gateway_main.cpp,
    // shared with ApiGateway.
    downstream.init_asio(&io);
    upstream.init_asio(&io);

    LinkMap& byDownstream = *new LinkMap();
    LinkMap& byUpstream   = *new LinkMap();
    size_t&  roundRobinNext = *new size_t(0);

    downstream.set_open_handler([&](ConnectionHandle downstreamHdl) {
        auto link = std::make_shared<Link>();
        link->downstreamHdl = downstreamHdl;

        // A hint from the API Gateway's REST room resolution beats
        // round-robin: it means "this exact shard already has the room I'm
        // about to ask for". Missing/unrecognized (old direct-WS flow, or
        // no room chosen yet) falls back to round-robin, same as before
        // this existed.
        std::string uri;
        websocketpp::lib::error_code hdrEc;
        Downstream::connection_ptr downCon = downstream.get_con_from_hdl(downstreamHdl, hdrEc);
        if (!hdrEc) {
            std::string hint = downCon->get_request_header("X-Shard-Hint");
            if (!hint.empty() && std::find(shardUris.begin(), shardUris.end(), hint) != shardUris.end())
                uri = hint;
        }
        if (uri.empty()) {
            uri = shardUris[roundRobinNext % shardUris.size()];
            ++roundRobinNext;
        }

        websocketpp::lib::error_code ec;
        Upstream::connection_ptr con = upstream.get_connection(uri, ec);
        if (ec) {
            spdlog::error("gateway: failed to create upstream connection: {}", ec.message());
            downstream.close(downstreamHdl, websocketpp::close::status::internal_endpoint_error, "upstream unavailable", ec);
            return;
        }
        link->upstreamHdl = con->get_handle();

        byDownstream[downstreamHdl]   = link;
        byUpstream[link->upstreamHdl] = link;

        upstream.connect(con);
        spdlog::info("gateway: client connected, routing to {}", uri);
    });

    downstream.set_message_handler([&](ConnectionHandle downstreamHdl, Downstream::message_ptr msg) {
        auto it = byDownstream.find(downstreamHdl);
        if (it == byDownstream.end()) return;
        LinkPtr link = it->second;

        if (link->upstreamOpen) {
            websocketpp::lib::error_code ec;
            upstream.send(link->upstreamHdl, msg->get_payload(), msg->get_opcode(), ec);
            if (ec) spdlog::error("gateway: forward to upstream failed: {}", ec.message());
        } else {
            link->pending.push_back(msg->get_payload());
        }
    });

    downstream.set_close_handler([&](ConnectionHandle downstreamHdl) {
        auto it = byDownstream.find(downstreamHdl);
        if (it == byDownstream.end()) return;
        LinkPtr link = it->second;

        websocketpp::lib::error_code ec;
        upstream.close(link->upstreamHdl, websocketpp::close::status::normal, "client disconnected", ec);

        byUpstream.erase(link->upstreamHdl);
        byDownstream.erase(it);
        spdlog::info("gateway: client disconnected");
    });

    upstream.set_open_handler([&](ConnectionHandle upstreamHdl) {
        auto it = byUpstream.find(upstreamHdl);
        if (it == byUpstream.end()) return;
        LinkPtr link = it->second;
        link->upstreamOpen = true;

        websocketpp::lib::error_code ec;
        for (const std::string& payload : link->pending) {
            upstream.send(upstreamHdl, payload, websocketpp::frame::opcode::text, ec);
            if (ec) spdlog::error("gateway: flushing queued message failed: {}", ec.message());
        }
        link->pending.clear();
        spdlog::info("gateway: upstream connected");
    });

    upstream.set_message_handler([&](ConnectionHandle upstreamHdl, Upstream::message_ptr msg) {
        auto it = byUpstream.find(upstreamHdl);
        if (it == byUpstream.end()) return;
        LinkPtr link = it->second;

        websocketpp::lib::error_code ec;
        downstream.send(link->downstreamHdl, msg->get_payload(), msg->get_opcode(), ec);
        if (ec) spdlog::error("gateway: forward to client failed: {}", ec.message());
    });

    // Same handler for both: whether the upstream connection dropped after
    // a clean open (on_close) or never managed to open at all (on_fail),
    // the real client's connection can't be salvaged either way.
    auto onUpstreamGone = [&](ConnectionHandle upstreamHdl) {
        auto it = byUpstream.find(upstreamHdl);
        if (it == byUpstream.end()) return;
        LinkPtr link = it->second;

        websocketpp::lib::error_code ec;
        downstream.close(link->downstreamHdl, websocketpp::close::status::normal, "upstream lost", ec);

        byDownstream.erase(link->downstreamHdl);
        byUpstream.erase(it);
        spdlog::info("gateway: upstream connection lost");
    };
    upstream.set_close_handler(onUpstreamGone);
    upstream.set_fail_handler(onUpstreamGone);

    downstream.listen(listenPort);
    downstream.start_accept();
    spdlog::info("gateway: WS listening on port {}, {} shard(s) configured", listenPort, shardUris.size());

    // No io.run() here - gateway_main.cpp calls it once, after ApiGateway
    // has set up its own listener on the same io_context.
}
