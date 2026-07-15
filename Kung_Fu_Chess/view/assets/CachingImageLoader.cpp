#include "CachingImageLoader.h"

Img CachingImageLoader::load(const std::string& location, const std::pair<int, int>& targetSize) {
    // Includes the requested size in the key - the same location loaded at
    // two different sizes is a different cached result.
    std::string key = location + "@" + std::to_string(targetSize.first) + "x" + std::to_string(targetSize.second);

    auto it = cache.find(key);
    if (it != cache.end())
        return it->second;

    return cache.emplace(key, inner.load(location, targetSize)).first->second;
}
