#include "app/Application.h"
#include <iostream>

int main() {
    try {
        Application app(std::cin);
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
