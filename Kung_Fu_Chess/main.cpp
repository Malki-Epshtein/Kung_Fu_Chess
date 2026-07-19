#include "server/io/BoardParser.h"
#include "client/app/GraphicalApplication.h"
#include "server/server_main.h"
#include "client/client_main.h"
#include <iostream>
#include <sstream>
#include <cstring>

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "--server") == 0)
        return server_main(argc, argv);
    if (argc >= 2 && std::strcmp(argv[1], "--client") == 0)
        return client_main(argc, argv);

    try {
        std::istringstream boardText(
            "Board:\n"
            "bR bN bB bQ bK bB bN bR\n"
            "bP bP bP bP bP bP bP bP\n"
            ". . . . . . . .\n"
            ". . . . . . . .\n"
            ". . . . . . . .\n"
            ". . . . . . . .\n"
            "wP wP wP wP wP wP wP wP\n"
            "wR wN wB wQ wK wB wN wR\n"
            "Commands:\n"
        );

        GraphicalApplication app(BoardParser::parseBoardOnly(boardText));
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
