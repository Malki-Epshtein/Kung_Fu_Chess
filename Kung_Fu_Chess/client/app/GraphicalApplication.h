#pragma once
#include "../input/Controller.h"
#include "../net/WsClient.h"
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

    WsClient client;

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

    static constexpr const char* SERVER_HOST       = "localhost";
    static constexpr uint16_t    SERVER_PORT        = 9002;

public:
    GraphicalApplication()
        : assetsRoot(readAssetsRoot()),
          pathBuilder(assetsRoot),
          cachingImageLoader(fileImageLoader),
          spriteRepository(pathBuilder, cachingImageLoader, boardScale),
          soundPlayer(assetsRoot),
          networkHandler(soundPlayer),
          controller(latestSnapshot, [this](const std::string& text) { client.send(text); }, boardScale),
          view(spriteRepository, assetsRoot, boardScale) {
        client.connect(SERVER_HOST, SERVER_PORT);

        // Shell login handshake (Stage F4) happens over this same
        // connection, before the game's own onMessage handler is installed
        // below - a separate login-only connection would grab a
        // White/Black seat and then immediately disconnect from it,
        // corrupting join-order role assignment on the server.
        LoginResult login = runLoginFlow(client);
        if (!login.success)
            throw std::runtime_error("login failed, exiting");

        // The game's own onMessage handler is installed later, in run() -
        // only once a room has actually been joined via the Home screen.
        // Until then, runHomeScreen() below needs the connection free to
        // run its own blocking CreateRoom/JoinRoom request/reply exchange
        // (same reasoning as login, just above). Real player names/elo
        // aren't known yet at this point (they come back from the room
        // join reply) - set in run() instead, once home.roomName is.
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
};
