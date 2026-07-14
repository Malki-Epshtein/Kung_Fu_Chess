#include "ImageView.h"
#include "../input/BoardMapper.h"
#include "../model/PieceStateMachine.h"
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <cctype>

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

    std::string stateDir(const std::string& assetsRoot, const std::string& pieceFolder, const std::string& stateFolder) {
        return assetsRoot + "/" + pieceFolder + "/states/" + stateFolder;
    }

    // A king is only ever missing from the snapshot once it has been
    // captured, so whichever color's king is still present is the winner.
    Chess::Color survivingKingColor(const GameSnapshot& snapshot) {
        bool whiteKing = false, blackKing = false;
        for (const auto& piece : snapshot.pieces) {
            if (piece.kind != Chess::Kind::King) continue;
            if (piece.color == Chess::Color::White) whiteKing = true;
            else if (piece.color == Chess::Color::Black) blackKing = true;
        }
        if (whiteKing && !blackKing) return Chess::Color::White;
        if (blackKing && !whiteKing) return Chess::Color::Black;
        return Chess::Color::None;
    }
}

// Reads the assets base path from view/paths_config.txt (once, cached) so a
// future folder move only requires editing that file, not this code.
const std::string& ImageView::getAssetsRoot() {
    if (!assetsRootLoaded) {
        std::ifstream file("view/paths_config.txt");
        if (!file)
            throw std::runtime_error("Cannot open view/paths_config.txt");
        std::getline(file, assetsRoot);
        while (!assetsRoot.empty() && std::isspace(static_cast<unsigned char>(assetsRoot.back())))
            assetsRoot.pop_back();
        assetsRootLoaded = true;
    }
    return assetsRoot;
}

//אן האנמציה חוזרת על עצמה, או לא
const AnimationConfig& ImageView::getConfig(const std::string& pieceFolder, const std::string& stateFolder) {
    std::string cacheKey = pieceFolder + "_" + stateFolder;

    auto it = configCache.find(cacheKey);
    if (it != configCache.end())
        return it->second;

    AnimationConfig config = loadAnimationConfig(stateDir(getAssetsRoot(), pieceFolder, stateFolder) + "/config.json");
    return configCache.emplace(cacheKey, config).first->second;
}

//כמה ספריטים יש לי
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
            probe.read(stateDir(getAssetsRoot(), pieceFolder, stateFolder) + "/sprites/" + std::to_string(count + 1) + ".png");
        } catch (const std::exception&) {
            break;
        }
        ++count;
    }

    frameCountCache[cacheKey] = count;
    return count;
}

//לוקח את הספריט המתאים לפי סוג החלק, צבעו, מצבו והמסגרת הנוכחית
Img& ImageView::getSprite(const std::string& pieceFolder, const std::string& stateFolder, int frameIndex) {
    std::string cacheKey = pieceFolder + "_" + stateFolder + "_" + std::to_string(frameIndex);

    auto it = spriteCache.find(cacheKey);
    if (it != spriteCache.end())
        return it->second;

    Img sprite;
    sprite.read(stateDir(getAssetsRoot(), pieceFolder, stateFolder) + "/sprites/" + std::to_string(frameIndex + 1) + ".png",
                 { BoardMapper::CELL_SIZE, BoardMapper::CELL_SIZE });

    return spriteCache.emplace(cacheKey, std::move(sprite)).first->second;
}

void ImageView::render(const GameSnapshot& snapshot) {
    if (!backgroundLoaded) {
        background.read(getAssetsRoot() + "/board.png");
        backgroundLoaded = true;
    }
    if (!canvasLoaded) {
        canvas.read(getAssetsRoot() + "/canvas.png");
        canvasLoaded = true;
    }

    // Fresh deep copy every frame - canvas is the cached, reusable "clean"
    // background (side panels + empty board area); draw_on mutates its
    // target in place, so drawing directly onto the cached Img would
    // permanently bake pieces/text into it.
    Img frame = canvas.clone();

    // Board sits centered, offset right by the left panel's width.
    background.draw_on(frame, PANEL_WIDTH, 0);

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
            // C++'s % keeps the sign of the dividend, so a negative rawIndex
            // (elapsed_in_state_ms briefly negative around a capture/rest
            // transition - root cause still being tracked down) could
            // otherwise produce a negative frame index and crash the sprite
            // lookup (sprites are named 1.png.. onward, never 0 or below).
            if (frameIndex < 0)
                frameIndex = (frameIndex % frameCount + frameCount) % frameCount;
        }

        // Render interpolation (Game Loop pattern): the piece's logical cell
        // only updates on arrival, but travelProgress (0-1, from GameEngine)
        // lets the draw position slide smoothly between origin and target
        // instead of jumping there when the motion resolves. PANEL_WIDTH
        // shifts everything right to match the now-centered board.
        int fromX = PANEL_WIDTH + piece.cell.col * BoardMapper::CELL_SIZE;
        int fromY = piece.cell.row * BoardMapper::CELL_SIZE;
        int toX   = PANEL_WIDTH + piece.targetCell.col * BoardMapper::CELL_SIZE;
        int toY   = piece.targetCell.row * BoardMapper::CELL_SIZE;
        int drawX = fromX + static_cast<int>((toX - fromX) * piece.travelProgress);
        int drawY = fromY + static_cast<int>((toY - fromY) * piece.travelProgress);

        Img& sprite = getSprite(folder, stateFolder, frameIndex);
        sprite.draw_on(frame, drawX, drawY);
    }

    // Side panels: Black on the left, White on the right - each shows its
    // own score and recent move history. Comes from Observer objects
    // (RealTimeArbiter -> observers -> here), never through GameSnapshot -
    // see the Step 9 plan. Uses put_text's default color so this file stays
    // free of cv:: types.
    const int leftPanelX  = 10;
    const int rightPanelX = PANEL_WIDTH + BOARD_WIDTH_PX + 15;
    const int lineHeight  = 20;
    const int maxLines    = 15;

    if (scoreObserver) {
        frame.put_text("Black", leftPanelX, 25, 0.7);
        frame.put_text("Score: " + std::to_string(scoreObserver->getScore(Chess::Color::Black)), leftPanelX, 55, 0.5);

        frame.put_text("White", rightPanelX, 25, 0.7);
        frame.put_text("Score: " + std::to_string(scoreObserver->getScore(Chess::Color::White)), rightPanelX, 55, 0.5);
    }

    if (moveLogObserver) {
        const auto& blackMoves = moveLogObserver->getMoves(Chess::Color::Black);
        int startBlack = std::max(0, static_cast<int>(blackMoves.size()) - maxLines);
        int y = 90;
        for (int i = startBlack; i < static_cast<int>(blackMoves.size()); ++i) {
            frame.put_text(blackMoves[i], leftPanelX, y, 0.35);
            y += lineHeight;
        }

        const auto& whiteMoves = moveLogObserver->getMoves(Chess::Color::White);
        int startWhite = std::max(0, static_cast<int>(whiteMoves.size()) - maxLines);
        y = 90;
        for (int i = startWhite; i < static_cast<int>(whiteMoves.size()); ++i) {
            frame.put_text(whiteMoves[i], rightPanelX, y, 0.35);
            y += lineHeight;
        }
    }

    // Spec (Iteration 9): display an end-of-game message when game_over is true.
    if (snapshot.game_over) {
        std::string message = "Game Over";
        Chess::Color winner = survivingKingColor(snapshot);
        if (winner == Chess::Color::White) message += " - White Wins!";
        else if (winner == Chess::Color::Black) message += " - Black Wins!";

        int boardPixelWidth  = snapshot.board_width  * BoardMapper::CELL_SIZE;
        int boardPixelHeight = snapshot.board_height * BoardMapper::CELL_SIZE;
        int textX = PANEL_WIDTH + boardPixelWidth / 2 - 180;
        int textY = boardPixelHeight / 2;
        frame.put_text(message, textX, textY, 1.0);
    }

    // Not using Img::show() here - it blocks on waitKey(0) every call, which
    // would freeze a live render loop. GraphicalApplication owns the event
    // pump (waitKey) and calls render() once per frame instead.
    cv::imshow(WINDOW_NAME, frame.get_mat());
}
