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

    void connect(const std::string& host, uint16_t port);
    void send(const std::string& text);
    void setOnMessage(std::function<void(const std::string&)> handler);
    void setOnOpen(std::function<void()> handler);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
