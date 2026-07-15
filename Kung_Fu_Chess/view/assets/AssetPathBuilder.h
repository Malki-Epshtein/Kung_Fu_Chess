#pragma once
#include "../../model/Piece.h"
#include <string>

// The ONLY class in this codebase that knows the on-disk asset layout -
// folder naming (e.g. "PW", "KB"), the "states"/"sprites" subfolders, and
// the "N.png"/"config.json" filenames. Pure string construction, no file
// I/O of its own - swapping to a different naming scheme, or an entirely
// different asset source (network, database), never requires touching
// anything outside this one class (and whichever code constructs it).
class AssetPathBuilder {
private:
    std::string assetsRoot;

    std::string stateDir(Chess::Kind kind, Chess::Color color, Chess::State state) const;

public:
    explicit AssetPathBuilder(std::string assetsRoot) : assetsRoot(std::move(assetsRoot)) {}

    std::string configPath(Chess::Kind kind, Chess::Color color, Chess::State state) const;
    std::string spritePath(Chess::Kind kind, Chess::Color color, Chess::State state, int frameIndex) const;
};
