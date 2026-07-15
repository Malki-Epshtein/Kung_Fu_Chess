#pragma once

// Pure, stateless: given how long a piece has been in its current animation
// state, which sprite frame should be shown. No file access, no caching - a
// self-contained calculation, easily unit-tested apart from asset loading.
int selectAnimationFrame(int elapsedMs, int framesPerSec, bool isLoop, int frameCount);
