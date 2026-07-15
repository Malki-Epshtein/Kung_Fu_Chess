#pragma once
#include "IImageLoader.h"

// The ONLY class that knows "loading an image" means "reading a file off
// local disk". A future network or database source is a sibling
// implementation of IImageLoader, not a change to this one.
class FileImageLoader : public IImageLoader {
public:
    Img load(const std::string& location, const std::pair<int, int>& targetSize = {}) override;
};
