#pragma once
#include "../input/Controller.h"
#include "../net/WsClient.h"
#include "../net/HttpClient.h"
#include "../net/NetworkMessageHandler.h"
#include "../view/ImageView.h"
#include "../view/BoardScale.h"
#include "../view/assets/SpriteRepository.h"
#include "../view/assets/AssetPathBuilder.h"
#include "../view/assets/FileImageLoader.h"
#include "../view/assets/CachingImageLoader.h"
#include "../view/assets/AssetsRootConfig.h"
#include "../audio/SoundPlayer.h"
#include "LoginFlow.h"
#include "HomeScreenView.h"
#include "../../shared/engine/GameSnapshot.h"
#include <cstdint>
#include <memory>
#include <stdexcept>

class GraphicalApplication {
private:
    // The board's current on-screen cell size - referenced by
    // SpriteRepository (sizes piece sprites to a cell), Controller (bounds
    // check + click-to-cell mapping), and ImageView (all board/piece/label
    // pixel math), so it's declared first: it has no dependencies of its
    // own, and everything that needs a reference to it must be declared
    // after it (C++ initializes members in declaration order, regardless
    // of the order written in the initializer list).
    BoardScale boardScale;

    std::string         assetsRoot;
    AssetPathBuilder     pathBuilder;
    FileImageLoader      fileImageLoader;
    CachingImageLoader   cachingImageLoader;
    SpriteRepository     spriteRepository;
    SoundPlayer          soundPlayer;

    // (Re)created by connect() below, once per WebSocket connection this
    // client ever opens - a plain WsClient can't be reconnected, and this
    // one has to be replaceable: Play's round-robin connection and a
    // Room's shard-pinned one are different connections, and even two Room
    // attempts can land on different shards. Null until the first
    // connect() call, which runHomeScreen() makes - no WebSocket exists
    // during login (see api below), unlike before the Gateway existed.
    std::unique_ptr<WsClient> client;

    // The HTTP Gateway connection (login/rooms) - separate from `client`,
    // which now carries only real-time gameplay traffic over a connection
    // pinned to one specific shard.
    HttpClient api;

    // Owns all inbound-message parsing/dispatch and the network-derived
    // state it produces (snapshot, score, move log, identity, disconnect
    // status) - see NetworkMessageHandler for why this was split out.
    // Needs soundPlayer to already exist (it plays move/capture/illegal
    // sounds as messages arrive).
    NetworkMessageHandler networkHandler;

    // A stable copy of the current snapshot, refreshed once per render
    // frame from networkHandler.currentState() - Controller holds a
    // reference to THIS member specifically (bound once, at construction),
    // so it must stay a real member here rather than a frame-local, and
    // must be updated in place (assignment) rather than replaced.
    GameSnapshot latestSnapshot{};

    Controller controller;
    ImageView  view;

    // Drag-to-resize state (Phase 3) - set on a LBUTTONDOWN that lands on
    // the handle (see BoardScale::isInHandle), consumed on MOUSEMOVE while
    // active, cleared on LBUTTONUP. Nothing here ever sends a network
    // message - resizing is purely local to this client.
    bool resizing_          = false;
    int  dragStartX_        = 0;
    int  dragStartY_        = 0;
    int  dragStartCellSize_ = 0;

    static constexpr const char* SERVER_HOST = "localhost";
    static constexpr uint16_t    SERVER_PORT = 9002; // WS Gateway
    static constexpr uint16_t    API_PORT    = 8081; // API Gateway (REST)

    // This connection's login token (from runLoginFlow, at construction) -
    // handed to every later REST call (POST /rooms, POST /rooms/{name}/join)
    // and to the WebSocket-side AUTH/EnterRoom messages, since LOGIN itself
    // no longer runs on any WebSocket connection.
    std::string token_;

public:
    GraphicalApplication()
        : assetsRoot(readAssetsRoot()),
          pathBuilder(assetsRoot),
          cachingImageLoader(fileImageLoader),
          spriteRepository(pathBuilder, cachingImageLoader, boardScale),
          soundPlayer(assetsRoot),
          api(SERVER_HOST, API_PORT),
          networkHandler(soundPlayer),
          controller(latestSnapshot, [this](const std::string& text) { client->send(text); }, boardScale),
          view(spriteRepository, assetsRoot, boardScale) {
        // Purely HTTP - no WebSocket exists yet at this point. The first
        // WebSocket connection isn't opened until run() calls
        // runHomeScreen(), which needs api/token_ to resolve a room (or a
        // round-robin shard for Play) before connecting.
        LoginResult login = runLoginFlow(api);
        if (!login.success)
            throw std::runtime_error("login failed, exiting");
        token_ = login.token;
    }

    void run();

    // Public only because it's called through the raw C function pointer
    // OpenCV's setMouseCallback requires (cast back from void* userdata) -
    // not meant to be called from anywhere else. Handles all three event
    // types the callback now forwards (down/move/up) - a plain click still
    // goes to onMouseClick below, unless it started a drag instead.
    void onMouseEvent(int event, int x, int y);

private:
    void onMouseClick(int x, int y);

    // The ConnectFn passed to runHomeScreen (see HomeScreenView.h) -
    // (re)creates `client`, connects with `shardHint` (empty for
    // round-robin), and blocks until the connection is actually open
    // before returning it. Same open-handshake wait LoginFlow used to do
    // for the old single persistent connection, now needed here instead
    // since a fresh connection can be made more than once per run.
    WsClient& connect(const std::string& shardHint);
};
