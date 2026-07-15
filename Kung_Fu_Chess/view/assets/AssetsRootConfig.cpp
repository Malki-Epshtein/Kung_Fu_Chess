#include "AssetsRootConfig.h"
#include <fstream>
#include <stdexcept>
#include <cctype>

std::string readAssetsRoot() {
    std::ifstream file("view/paths_config.txt");
    if (!file)
        throw std::runtime_error("Cannot open view/paths_config.txt");

    std::string root;
    std::getline(file, root);
    while (!root.empty() && std::isspace(static_cast<unsigned char>(root.back())))
        root.pop_back();
    return root;
}
