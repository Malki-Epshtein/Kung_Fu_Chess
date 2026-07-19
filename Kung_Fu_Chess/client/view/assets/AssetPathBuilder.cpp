#include "AssetPathBuilder.h"
#include "../../../shared/model/PieceStateMachine.h"
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
            default: throw std::invalid_argument("No sprite folder for this piece kind");
        }
        char colorChar = (color == Chess::Color::White) ? 'w' : 'b';
        return std::string(1, colorChar) + std::string(1, kindChar);
    }
}

std::string AssetPathBuilder::stateDir(Chess::Kind kind, Chess::Color color, Chess::State state) const {
    return assetsRoot + "/" + pieceFolder(kind, color) + "/states/" + stateToFolderName(state);
}

std::string AssetPathBuilder::configPath(Chess::Kind kind, Chess::Color color, Chess::State state) const {
    return stateDir(kind, color, state) + "/config.json";
}

std::string AssetPathBuilder::spritePath(Chess::Kind kind, Chess::Color color, Chess::State state, int frameIndex) const {
    return stateDir(kind, color, state) + "/sprites/" + std::to_string(frameIndex + 1) + ".png";
}
