#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

// Long-lived WebSocket client with its own dedicated network I/O thread -
// deliberately NOT tied to the caller's own loop (e.g. a GUI render loop),
// so network processing keeps running at full speed even if the caller's
// thread gets throttled (e.g. Windows deprioritizing an unfocused window's
// message loop). connect() starts the thread; send() is safe to call from
// any thread. setOnMessage's handler fires on the network thread - the
// caller is responsible for synchronizing whatever it does with that data.
class WsClient {
public:
    WsClient();
    ~WsClient();

    // `shardHint`, if non-empty, is sent as the "X-Shard-Hint" header on the
    // WebSocket handshake - the Gateway (WsGateway) routes directly to that
    // shard instead of round-robining, which matters once a REST call has
    // already resolved a specific shard for the room being joined (see
    // HomeScreenView's sendRoomRequest). Empty for the plain round-robin
    // case (initial connect, or Play/FindGame, where no room is known yet).
    void connect(const std::string& host, uint16_t port, const std::string& shardHint = "");
    void send(const std::string& text);
    void setOnMessage(std::function<void(const std::string&)> handler);
    void setOnOpen(std::function<void()> handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
