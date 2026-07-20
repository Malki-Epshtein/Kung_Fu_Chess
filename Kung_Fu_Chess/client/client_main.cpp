#include "app/GraphicalApplication.h"
#include <iostream>

int main(int /*argc*/, char** /*argv*/) {
    try {
        GraphicalApplication app;
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
