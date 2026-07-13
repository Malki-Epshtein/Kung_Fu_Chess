#pragma once
#include <string>
#include <vector>

enum class ScriptCommandType { Click, Jump, Wait, PrintBoard };

struct ScriptCommand {
    ScriptCommandType        type;
    int                      x = 0;  // עבור Click / Jump
    int                      y = 0;  // עבור Click / Jump
    int                      ms = 0; // עבור Wait
    std::vector<std::string> expectedBoard; // עבור PrintBoard - השורות הצפויות שאחריו בסקריפט
};
