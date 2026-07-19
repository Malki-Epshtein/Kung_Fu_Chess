#define _CRT_SECURE_NO_WARNINGS
#include "Application.h"

void Application::run() {
    std::string line;
    while (std::getline(input, line)) {
        if (line.find("click") == 0) {
            int x, y;
            sscanf(line.c_str(), "click %d %d", &x, &y);
            controller.handleMouseClick(x, y);
        }
        else if (line.find("jump") == 0) {
            int x, y;
            sscanf(line.c_str(), "jump %d %d", &x, &y);
            controller.handleJump(x, y);
        }
        else if (line.find("wait") == 0) {
            int ms;
            sscanf(line.c_str(), "wait %d", &ms);
            controller.handleWait(ms);
        }
        else if (line == "print board") {
            BoardPrinter::print(controller.getSnapshot(), std::cout);
        }
    }
}
