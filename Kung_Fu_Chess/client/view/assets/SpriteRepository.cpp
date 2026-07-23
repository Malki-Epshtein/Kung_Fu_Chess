#include "SpriteRepository.h"
#include "AnimationFrameSelector.h"

const AnimationConfig& SpriteRepository::getConfig(const StateKey& key) {
    auto it = configCache.find(key);
    if (it != configCache.end())
        return it->second;

    AnimationConfig config = loadAnimationConfig(pathBuilder.configPath(key.kind, key.color, key.state));
    return configCache.emplace(key, config).first->second;
}

Img SpriteRepository::getSprite(Chess::Kind kind, Chess::Color color, Chess::State state, int elapsedMs) {
    StateKey stateKey{ kind, color, state };
    const AnimationConfig& config = getConfig(stateKey);
    int frameIndex = selectAnimationFrame(elapsedMs, config.frames_per_sec, config.is_loop, config.frame_count);

    std::string path = pathBuilder.spritePath(kind, color, state, frameIndex);
    int cellSize = boardScale.cellSize();
    return imageLoader.load(path, { cellSize, cellSize });
}
