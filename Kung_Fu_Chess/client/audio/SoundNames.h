#pragma once

// Every sound filename SoundPlayer::play() is called with, in one place -
// these used to be raw string literals scattered across GraphicalApplication
// and NetworkMessageHandler (whichever file first needed a given sound).
// Filenames only, not paths - SoundPlayer itself owns the "assetsRoot +
// /sounds/" prefix (see SoundPlayer::play).
namespace Sounds {
    constexpr const char* CLICK        = "click.wav";
    constexpr const char* MOVE         = "move.wav";
    constexpr const char* CAPTURE      = "capture.wav";
    constexpr const char* ILLEGAL_MOVE = "illegal_move.wav";
    constexpr const char* JUMP         = "jump.wav";
    constexpr const char* GAME_OVER    = "game over.wav";
}
