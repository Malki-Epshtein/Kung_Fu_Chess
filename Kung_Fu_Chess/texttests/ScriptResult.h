#pragma once
#include <string>

struct ScriptResult {
    bool        passed;
    std::string message; // ריק אם עבר; אחרת expected מול actual
};
