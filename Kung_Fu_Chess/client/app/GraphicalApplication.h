#pragma once
#include "../input/Controller.h"
#include "../net/WsClient.h"
#include "../view/ImageView.h"
#include "../view/assets/SpriteRepository.h"
#include "../view/assets/AssetPathBuilder.h"
#include "../view/assets/FileImageLoader.h"
#include "../view/assets/CachingImageLoader.h"
#include "../view/assets/AssetsRootConfig.h"
#include "../../shared/engine/GameSnapshot.h"
#include <cstdint>
#include <mutex>

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
          controller(latestSnapshot, [this](const std::string& text) { client.send(text); }),
          view(spriteRepository, assetsRoot) {
        client.setOnMessage([this](const std::string& text) { onMessage(text); });
        client.connect(SERVER_HOST, SERVER_PORT);
        view.setPlayerNames(WHITE_PLAYER_NAME, BLACK_PLAYER_NAME);
    }

    void run();

private:
    void onMessage(const std::string& text);
};
