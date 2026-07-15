#include "AnimationFrameSelector.h"
#include <algorithm>

int selectAnimationFrame(int elapsedMs, int framesPerSec, bool isLoop, int frameCount) {
    if (frameCount <= 0 || framesPerSec <= 0)
        return 0;

    int rawIndex = (elapsedMs * framesPerSec) / 1000;
    int index = isLoop ? (rawIndex % frameCount)
                        : std::min(rawIndex, frameCount - 1);

    // C++'s % keeps the sign of the dividend, so a negative rawIndex
    // (elapsed briefly negative around a capture/rest transition - a
    // real-time timing edge case) could otherwise produce a negative index.
    if (index < 0)
        index = (index % frameCount + frameCount) % frameCount;

    return index;
}
