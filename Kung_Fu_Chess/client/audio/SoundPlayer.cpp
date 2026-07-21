#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include "SoundPlayer.h"

void SoundPlayer::play(const std::string& fileName) const {
    // SND_ASYNC: don't block the calling thread while it plays.
    // SND_NODEFAULT: a missing/bad file stays silent instead of falling
    // back to the Windows default system sound.
    std::string path = assetsRoot_ + "/sounds/" + fileName;
    PlaySoundA(path.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}
