#include "ImageView.h"
#include "../input/BoardMapper.h"
#include "../model/PieceStateMachine.h"
#include <algorithm>
#include <stdexcept>

namespace {
    std::string pieceFolder(Chess::Kind kind, Chess::Color color) {
        char kindChar;
        switch (kind) {
            case Chess::Kind::King:   kindChar = 'K'; break;
            case Chess::Kind::Queen:  kindChar = 'Q'; break;
            case Chess::Kind::Rook:   kindChar = 'R'; break;
            case Chess::Kind::Bishop: kindChar = 'B'; break;
            case Chess::Kind::Knight: kindChar = 'N'; break;
            case Chess::Kind::Pawn:   kindChar = 'P'; break;
            default: return "";
        }
        char colorChar = (color == Chess::Color::White) ? 'W' : 'B';
        return std::string(1, kindChar) + std::string(1, colorChar);
    }

    std::string stateDir(const std::string& pieceFolder, const std::string& stateFolder) {
        return "view/img/" + pieceFolder + "/states/" + stateFolder;
    }
}

const AnimationConfig& ImageView::getConfig(const std::string& pieceFolder, const std::string& stateFolder) {
    std::string cacheKey = pieceFolder + "_" + stateFolder;

    auto it = configCache.find(cacheKey);
    if (it != configCache.end())
        return it->second;

    AnimationConfig config = loadAnimationConfig(stateDir(pieceFolder, stateFolder) + "/config.json");
    return configCache.emplace(cacheKey, config).first->second;
}

int ImageView::getFrameCount(const std::string& pieceFolder, const std::string& stateFolder) {
    std::string cacheKey = pieceFolder + "_" + stateFolder;

    auto it = frameCountCache.find(cacheKey);
    if (it != frameCountCache.end())
        return it->second;

    // No C++17 <filesystem> available in this project's build config, so
    // frames are counted by probing sequential filenames (1.png, 2.png, ...)
    // until Img::read() throws for a missing file - reusing its existing
    // "file not found" behavior instead of adding a directory-listing API.
    int count = 0;
    while (true) {
        try {
            Img probe;
            probe.read(stateDir(pieceFolder, stateFolder) + "/sprites/" + std::to_string(count + 1) + ".png");
        } catch (const std::exception&) {
            break;
        }
        ++count;
    }

    frameCountCache[cacheKey] = count;
    return count;
}

Img& ImageView::getSprite(const std::string& pieceFolder, const std::string& stateFolder, int frameIndex) {
    std::string cacheKey = pieceFolder + "_" + stateFolder + "_" + std::to_string(frameIndex);

    auto it = spriteCache.find(cacheKey);
    if (it != spriteCache.end())
        return it->second;

    Img sprite;
    sprite.read(stateDir(pieceFolder, stateFolder) + "/sprites/" + std::to_string(frameIndex + 1) + ".png",
                 { BoardMapper::CELL_SIZE, BoardMapper::CELL_SIZE });

    return spriteCache.emplace(cacheKey, std::move(sprite)).first->second;
}

void ImageView::render(const GameSnapshot& snapshot) {
    if (!backgroundLoaded) {
        background.read("view/img/board.png");
        backgroundLoaded = true;
    }

    // Fresh deep copy every frame - background is the one cached, reusable
    // "clean" image; draw_on mutates its target in place, so drawing
    // directly onto the cached Img would permanently bake pieces into it.
    Img frame = background.clone();

    for (const auto& piece : snapshot.pieces) {
        std::string folder = pieceFolder(piece.kind, piece.color);
        if (folder.empty())
            continue;

        std::string stateFolder = stateToFolderName(piece.state);
        const AnimationConfig& config = getConfig(folder, stateFolder);
        int frameCount = getFrameCount(folder, stateFolder);

        int frameIndex = 0;
        if (frameCount > 0 && config.frames_per_sec > 0) {
            int rawIndex = (piece.elapsed_in_state_ms * config.frames_per_sec) / 1000;
            frameIndex = config.is_loop ? (rawIndex % frameCount)
                                         : std::min(rawIndex, frameCount - 1);
        }

        Img& sprite = getSprite(folder, stateFolder, frameIndex);
        sprite.draw_on(frame, piece.cell.col * BoardMapper::CELL_SIZE,
                              piece.cell.row * BoardMapper::CELL_SIZE);
    }

    // Not using Img::show() here - it blocks on waitKey(0) every call, which
    // would freeze a live render loop. GraphicalApplication owns the event
    // pump (waitKey) and calls render() once per frame instead.
    cv::imshow(WINDOW_NAME, frame.get_mat());
}
