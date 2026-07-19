#pragma once
#include <string>

// Reads the assets base path from client/view/paths_config.txt - the single place
// that resolves this, shared by everything that needs to locate assets
// (AssetPathBuilder for sprites, ImageView for the board/canvas images).
std::string readAssetsRoot();
