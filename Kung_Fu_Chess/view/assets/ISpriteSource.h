#pragma once
#include "../src/img.hpp"
#include "../../model/Piece.h"

// Abstraction ImageView renders against, so it never needs to know how or
// where sprites actually come from (files on disk, an in-memory fake for
// tests, ...) - Dependency Inversion: the view depends on this interface,
// not on SpriteRepository directly.
class ISpriteSource {
public:
    // Returned by value: Img's copy is cheap (shares the underlying cv::Mat
    // buffer via refcounting, see Img::clone()'s doc comment).
    virtual Img getSprite(Chess::Kind kind, Chess::Color color, Chess::State state, int elapsedMs) = 0;
    virtual ~ISpriteSource() = default;
};
