#pragma once
#include "IImageLoader.h"
#include <unordered_map>

// Decorator: wraps any IImageLoader and caches its results by location
// string, so the wrapped loader is only ever asked for the same image
// once. Works identically no matter what the wrapped loader actually is
// (file, network, database) - caching is a cross-cutting concern that has
// nothing to do with where images come from.
class CachingImageLoader : public IImageLoader {
private:
    IImageLoader& inner;
    std::unordered_map<std::string, Img> cache;

public:
    explicit CachingImageLoader(IImageLoader& inner) : inner(inner) {}

    Img load(const std::string& location, const std::pair<int, int>& targetSize = {}) override;
};
