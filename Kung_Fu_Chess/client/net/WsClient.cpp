#include "WsClient.h"
#include "../../shared/log/Log.h"
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <mutex>
#include <thread>

struct WsClient::Impl {
    using WsppClient = websocketpp::client<websocketpp::config::asio_client>;

    WsppClient                              client;//באמת זה הוא שגורם לכל החיבור
    websocketpp::connection_hdl             hdl;//החיבור עצמו
    // Guards onMessage/onOpen themselves - setOnMessage/setOnOpen are
    // called from the caller's own thread (main/GUI), while the network
    // thread reads and invokes them from the websocketpp handlers below.
    // Without this, a setOnMessage(...) call racing the network thread's
    // read-and-invoke of the PREVIOUS handler is a real data race on the
    // std::function itself (observed crash: "unlock of unowned mutex" -
    // the old handler's captured state being torn down mid-invocation).
    std::mutex                              handlerMutex;
    std::function<void(const std::string&)> onMessage;//מקום שאפשר לשים פונקציה שתתבצע כאשר מתקבלת הודעה מהשרת
    std::function<void()>                   onOpen;//מקום שאפשר לשים פונקציה שתתבצע כאשר החיבור נפתח
    bool                                    connected = false;//אם החיבור פעיל או לא
    std::thread                             networkThread;//ת'רד נפרד שמריץ את הלולאה של הרשת

    Impl() {
        client.clear_access_channels(websocketpp::log::alevel::all);
        client.clear_error_channels(websocketpp::log::elevel::all);
        client.init_asio();
    }

    ~Impl() {
        client.get_io_service().stop();
        if (networkThread.joinable())
            networkThread.join();
    }
};

WsClient::WsClient() : impl(std::make_unique<Impl>()) {}
WsClient::~WsClient() = default;

void WsClient::connect(const std::string& host, uint16_t port, const std::string& shardHint) {//הפונקציה שמתחברת לשרת
    auto& client = impl->client;

    client.set_open_handler([this](websocketpp::connection_hdl hdl) {//כאשר החיבור נפתח
        impl->hdl = hdl;
        impl->connected = true;
        spdlog::info("connected");
        // Copy the handler out under the lock, then call it unlocked - so
        // the handler itself (arbitrary caller code) is never invoked
        // while handlerMutex is held.
        std::function<void()> handler;
        {
            std::lock_guard<std::mutex> lock(impl->handlerMutex);
            handler = impl->onOpen;
        }
        if (handler)//אם יש פונקציה שהוגדרה על ידי המשתמש, היא תתבצע
            handler();
    });
    client.set_message_handler([this](websocketpp::connection_hdl, Impl::WsppClient::message_ptr msg) {//כאשר מתקבלת הודעה מהשרת
        std::function<void(const std::string&)> handler;
        {
            std::lock_guard<std::mutex> lock(impl->handlerMutex);
            handler = impl->onMessage;
        }
        if (handler)
            handler(msg->get_payload());
    });
    client.set_fail_handler([this](websocketpp::connection_hdl hdl) {//כאשר החיבור נכשל
        spdlog::error("connection failed: {}", impl->client.get_con_from_hdl(hdl)->get_ec().message());
    });
    client.set_close_handler([this](websocketpp::connection_hdl) {
        impl->connected = false;
        spdlog::info("connection closed");
    });

    websocketpp::lib::error_code ec;
    std::string uri = "ws://" + host + ":" + std::to_string(port);
    auto con = client.get_connection(uri, ec);
    if (ec) {
        spdlog::error("could not create connection: {}", ec.message());
        return;
    }
    if (!shardHint.empty())
        con->append_header("X-Shard-Hint", shardHint);
    client.connect(con);//כאן זה עדיין לא באמת מתחבר הסרד יחבר

    // Dedicated thread, entirely separate from any window/message loop the
    // caller might have - this is the whole point (see header comment).
    impl->networkThread = std::thread([this]() {
        impl->client.get_io_service().run();
    });
}

void WsClient::send(const std::string& text) {//שליחת הודעה לשרת
    // Marshal the actual send onto the network thread via asio::post, so
    // this is safe to call from any thread (e.g. the GUI thread on a click).
    impl->client.get_io_service().post([this, text]() {
        if (!impl->connected)
            return;
        websocketpp::lib::error_code ec;
        impl->client.send(impl->hdl, text, websocketpp::frame::opcode::text, ec);//פה באמת נשלח ההודעה
        if (ec)
            spdlog::error("send failed: {}", ec.message());
        else
            // The one chokepoint every outgoing message passes through
            // regardless of caller (login, room join, moves, ...) - traces
            // full outbound traffic without needing a log line in every
            // individual call site.
            spdlog::debug("sending: {}", text);
    });
}

void WsClient::setOnMessage(std::function<void(const std::string&)> handler) {//הגדרת פונקציה שתתבצע כאשר מתקבלת הודעה מהשרת מי יטפל בהודעה
    std::lock_guard<std::mutex> lock(impl->handlerMutex);
    impl->onMessage = std::move(handler);
}

void WsClient::setOnOpen(std::function<void()> handler) {// מי יטפל כאשר החיבור נפתח
    std::lock_guard<std::mutex> lock(impl->handlerMutex);
    impl->onOpen = std::move(handler);
}
