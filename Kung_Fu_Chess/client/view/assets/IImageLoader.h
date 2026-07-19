#pragma once
#include "../src/img.hpp"
#include <string>

// Fetches and decodes one image given an opaque location string (a file
// path today; could be a URL or a database key tomorrow - this interface
// doesn't care). Deliberately separate from ISpriteSource: this is the
// "how do I turn a location into pixels" concern, not "which piece/state/
// frame maps to which location" (that's AssetPathBuilder's job) or "which
// sprite does this piece need right now" (that's SpriteRepository's job).
class IImageLoader {
public:
    // Returned by value: Img's copy is cheap (shares the underlying cv::Mat
    // buffer via refcounting, see Img::clone()'s doc comment) - so a cache
    // or a decorator can hand back a stored image without exposing a
    // reference whose lifetime the caller would have to reason about.
    //
    // targetSize: Img has no separate resize step (draw_on always draws at
    // the source's native resolution), so a caller that needs a specific
    // pixel size - sprites must exactly fill one board cell - has to ask
    // for it at load time. Empty means "keep the image's native size".
    virtual Img load(const std::string& location, const std::pair<int, int>& targetSize = {}) = 0;
    virtual ~IImageLoader() = default;
};
