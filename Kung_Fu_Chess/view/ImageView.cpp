#include "ImageView.h"
#include "../input/BoardMapper.h"
#include "../model/PieceStateMachine.h"

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
}

Img& ImageView::getSprite(const std::string& pieceFolder, const std::string& stateFolder) {
    std::string cacheKey = pieceFolder + "_" + stateFolder;

    auto it = spriteCache.find(cacheKey);
    if (it != spriteCache.end())
        return it->second;

    Img sprite;
    sprite.read("view/img/" + pieceFolder + "/states/" + stateFolder + "/sprites/1.png",
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
        Img& sprite = getSprite(folder, stateFolder);
        sprite.draw_on(frame, piece.cell.col * BoardMapper::CELL_SIZE,
                              piece.cell.row * BoardMapper::CELL_SIZE);
    }

    // Not using Img::show() here - it blocks on waitKey(0) every call, which
    // would freeze a live render loop. GraphicalApplication owns the event
    // pump (waitKey) and calls render() once per frame instead.
    cv::imshow(WINDOW_NAME, frame.get_mat());
}
