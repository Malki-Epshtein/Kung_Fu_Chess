#pragma once
#include "ISpriteSource.h"
#include "AssetPathBuilder.h"
#include "IImageLoader.h"
#include "AnimationConfig.h"
#include <unordered_map>

// Orchestrates turning a piece's (kind, color, state, elapsed time) into
// the right Img: asks AssetPathBuilder where things are, asks the injected
// IImageLoader to actually fetch them, and asks AnimationFrameSelector
// (pure function) which frame to pick. Owns none of those concerns itself -
// this class is glue, not implementation. It still loads/caches
// AnimationConfig directly: that's config.json/JSON-format knowledge
// specific to the file-based setup, not (yet) worth a second swappable
// abstraction until there's a real second source to design it against.
class SpriteRepository : public ISpriteSource {
private:
    struct StateKey {
        Chess::Kind  kind;
        Chess::Color color;
        Chess::State state;
        bool operator==(const StateKey& other) const {
            return kind == other.kind && color == other.color && state == other.state;
        }
    };
    struct StateKeyHash {
        size_t operator()(const StateKey& k) const {
            size_t h = std::hash<int>()(static_cast<int>(k.kind));
            h = h * 31 + std::hash<int>()(static_cast<int>(k.color));
            h = h * 31 + std::hash<int>()(static_cast<int>(k.state));
            return h;
        }
    };

    AssetPathBuilder& pathBuilder;
    IImageLoader&     imageLoader;
    std::unordered_map<StateKey, AnimationConfig, StateKeyHash> configCache;

    const AnimationConfig& getConfig(const StateKey& key);

public:
    SpriteRepository(AssetPathBuilder& pathBuilder, IImageLoader& imageLoader)
        : pathBuilder(pathBuilder), imageLoader(imageLoader) {}

    Img getSprite(Chess::Kind kind, Chess::Color color, Chess::State state, int elapsedMs) override;
};
