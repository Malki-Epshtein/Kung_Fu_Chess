#include "server/server_main.h"
#include "client/app/GraphicalApplication.h"
#include <cstring>
#include <iostream>

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "--server") == 0)
        return server_main(argc, argv);

    // "--client" or no flags at all: launch the real graphical client.
    try {
        GraphicalApplication app;
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
