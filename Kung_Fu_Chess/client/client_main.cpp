#include "app/GraphicalApplication.h"
#include "../shared/log/Log.h"

int main(int /*argc*/, char** /*argv*/) {
    Log::init("client");
    try {
        GraphicalApplication app;
        app.run();
    }
    catch (const std::exception& e) {
        spdlog::error("{}", e.what());
    }
    return 0;
}
