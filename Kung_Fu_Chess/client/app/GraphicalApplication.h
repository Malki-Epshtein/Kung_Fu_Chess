#pragma once
#include "../input/Controller.h"
#include "../net/WsClient.h"
#include "../view/ImageView.h"
#include "../view/assets/SpriteRepository.h"
#include "../view/assets/AssetPathBuilder.h"
#include "../view/assets/FileImageLoader.h"
#include "../view/assets/CachingImageLoader.h"
#include "../view/assets/AssetsRootConfig.h"
#include "../audio/SoundPlayer.h"
#include "LoginFlow.h"
#include "HomeScreenView.h"
#include "../../shared/engine/GameSnapshot.h"
#include "../../shared/protocol/MoveLogCodec.h"
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <vector>

class GraphicalApplication {
private:
    // Declaration order matters here: each of these is built from the one
    // before it in the constructor's initializer list, so it must also be
    // declared after it (C++ initializes members in declaration order,
    // regardless of the order written in the initializer list).
    std::string         assetsRoot;
    AssetPathBuilder     pathBuilder;
    FileImageLoader      fileImageLoader;
    CachingImageLoader   cachingImageLoader;
    SpriteRepository     spriteRepository;
    SoundPlayer          soundPlayer;

    WsClient client;

    // networkSnapshot is written by WsClient's onMessage callback on its own
    // network thread, guarded by snapshotMutex. latestSnapshot is a stable
    // copy taken under that lock once per render frame, on the GUI thread -
    // Controller holds a reference to latestSnapshot only, so it never needs
    // to know threading is involved at all.
    std::mutex   snapshotMutex;
    GameSnapshot networkSnapshot{};
    GameSnapshot latestSnapshot{};

    // Stage D: disconnect grace-period countdown, written by the network
    // thread under the same lock, copied out once per frame like the
    // snapshot above.
    bool        networkDisconnectActive = false;
    std::string networkDisconnectMessage;

    // Stage I: score rides on the same snapshot broadcast (extracted here
    // rather than through GameSnapshotCodec, same as disconnect above).
    // Move log arrives as separate MOVE_LOGGED pushes instead - only ever
    // appended to, never replaced - plus a one-time backfill seeded from
    // the room-join reply's moveLog field before the game window opens.
    int                     networkWhiteScore = 0;
    int                     networkBlackScore = 0;
    std::vector<MoveEntry>  networkWhiteMoves;
    std::vector<MoveEntry>  networkBlackMoves;

    Controller controller;
    ImageView  view;

    // No text-input UI exists yet, so player names are fixed here for now -
    // easy to swap for a config-file read later without touching callers.
    static constexpr const char* WHITE_PLAYER_NAME = "White Player";
    static constexpr const char* BLACK_PLAYER_NAME = "Black Player";
    static constexpr const char* SERVER_HOST       = "localhost";
    static constexpr uint16_t    SERVER_PORT        = 9002;

public:
    GraphicalApplication()
        : assetsRoot(readAssetsRoot()),
          pathBuilder(assetsRoot),
          cachingImageLoader(fileImageLoader),
          spriteRepository(pathBuilder, cachingImageLoader),
          soundPlayer(assetsRoot),
          controller(latestSnapshot, [this](const std::string& text) { client.send(text); }),
          view(spriteRepository, assetsRoot) {
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
        // (same reasoning as login, just above).
        view.setPlayerNames(WHITE_PLAYER_NAME, BLACK_PLAYER_NAME);
    }

    void run();

private:
    void onMessage(const std::string& text);
};
