#pragma once
#include <optional>

struct CpuMetrics {
    std::optional<double> usage;
    // Mean OS-reported frequency across logical processors, in GHz.
    std::optional<double> currentFrequency;
};
