#pragma once
#include <string>

// Single job: given a sound file's name (e.g. "capture.wav"), play it from
// assetsRoot/sounds/. Every client-side event that wants a sound effect
// goes through this one class instead of calling the Windows audio API
// directly, so PlaySound's flags/behavior live in exactly one place.
class SoundPlayer {
public:
    explicit SoundPlayer(std::string assetsRoot) : assetsRoot_(std::move(assetsRoot)) {}

    // Non-blocking - safe to call from the network thread (GraphicalApplication::onMessage)
    // as well as the GUI thread, since playback never waits on this call.
    void play(const std::string& fileName) const;

private:
    std::string assetsRoot_;
};
