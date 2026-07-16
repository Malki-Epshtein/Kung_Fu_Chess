#pragma once
#include "../engine/GameEngine.h"
#include "../engine/ScoreObserver.h"
#include "../engine/MoveLogObserver.h"
#include "../input/Controller.h"
#include "../view/ImageView.h"
#include "../view/assets/SpriteRepository.h"
#include "../view/assets/AssetPathBuilder.h"
#include "../view/assets/FileImageLoader.h"
#include "../view/assets/CachingImageLoader.h"
#include "../view/assets/AssetsRootConfig.h"
#include "../model/Board.h"
#include <memory>

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

    GameEngine      engine;
    Controller      controller;
    ImageView       view;
    ScoreObserver   scoreObserver;
    MoveLogObserver moveLogObserver;

    // No text-input UI exists yet, so player names are fixed here for now -
    // easy to swap for a config-file read later without touching callers.
    static constexpr const char* WHITE_PLAYER_NAME = "White Player";
    static constexpr const char* BLACK_PLAYER_NAME = "Black Player";

public:
    GraphicalApplication(std::shared_ptr<Board> board)
        : assetsRoot(readAssetsRoot()),
          pathBuilder(assetsRoot),
          cachingImageLoader(fileImageLoader),
          spriteRepository(pathBuilder, cachingImageLoader),
          engine(board, /*simultaneousMode=*/true),
          controller(engine),
          view(spriteRepository, assetsRoot),
          moveLogObserver(board->getHeight()) {
        engine.addCaptureObserver(&scoreObserver);
        engine.addMoveObserver(&moveLogObserver);
        view.setScoreObserver(&scoreObserver);
        view.setMoveLogObserver(&moveLogObserver);
        view.setPlayerNames(WHITE_PLAYER_NAME, BLACK_PLAYER_NAME);
    }

    void run();
};
