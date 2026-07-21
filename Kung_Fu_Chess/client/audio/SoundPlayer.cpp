#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include "SoundPlayer.h"
#include <fstream>
#include <cstring>
#include <cctype>

namespace {
    // Trust the file's actual bytes, not its extension - an asset can be
    // named "x.wav" while still holding mp3 data inside (a rename doesn't
    // transcode audio).
    bool isRealWav(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        char header[4] = {};
        file.read(header, sizeof(header));
        return file.gcount() == sizeof(header) && std::memcmp(header, "RIFF", 4) == 0;
    }

    // MCI aliases must be simple identifiers - derive one from the file
    // name so each distinct sound gets its own independent MCI device
    // instance. This is what lets two different sounds (e.g. a move and
    // the capture it caused) play at the same time instead of the second
    // one cutting off the first - a single shared alias/PlaySound's
    // single-channel behavior can only ever play one sound system-wide.
    std::string aliasFor(const std::string& fileName) {
        std::string alias = "kfc_";
        for (char c : fileName)
            if (std::isalnum(static_cast<unsigned char>(c)))
                alias += c;
        return alias;
    }
}

void SoundPlayer::play(const std::string& fileName) const {
    std::string path = assetsRoot_ + "/sounds/" + fileName;
    std::string alias = aliasFor(fileName);
    const char* deviceType = isRealWav(path) ? "waveaudio" : "MPEGVideo";

    // Close this event's own previous instance first (a no-op, ignored,
    // if it wasn't open) so replaying the same event restarts cleanly -
    // other events' aliases are untouched and keep playing.
    mciSendStringA(("close " + alias).c_str(), NULL, 0, NULL);
    std::string openCmd = "open \"" + path + "\" type " + deviceType + " alias " + alias;
    if (mciSendStringA(openCmd.c_str(), NULL, 0, NULL) == 0)
        mciSendStringA(("play " + alias).c_str(), NULL, 0, NULL);
}
