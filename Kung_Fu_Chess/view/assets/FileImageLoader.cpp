#include "FileImageLoader.h"

Img FileImageLoader::load(const std::string& location, const std::pair<int, int>& targetSize) {
    Img image;
    image.read(location, targetSize);
    return image;
}
