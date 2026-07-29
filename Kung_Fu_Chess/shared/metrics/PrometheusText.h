#pragma once
#include <string>

// Minimal Prometheus text exposition format - just enough for one gauge or
// counter per line (HELP+TYPE included so a real Prometheus scrape target
// parses it correctly). No histograms/labels: a real client library
// (prometheus-cpp) would be a new vendored dependency for no real benefit
// at this scale.
inline std::string gaugeMetric(const std::string& name, const std::string& help, double value) {
    return "# HELP " + name + " " + help + "\n"
         + "# TYPE " + name + " gauge\n"
         + name + " " + std::to_string(value) + "\n";
}

// Same shape as gaugeMetric, just labeled "counter" - for values that only
// ever accumulate (e.g. matches found, allocations made), as opposed to a
// live snapshot like a gauge. The caller is responsible for actually only
// incrementing, never decrementing, whatever it passes in here.
inline std::string counterMetric(const std::string& name, const std::string& help, double value) {
    return "# HELP " + name + " " + help + "\n"
         + "# TYPE " + name + " counter\n"
         + name + " " + std::to_string(value) + "\n";
}
