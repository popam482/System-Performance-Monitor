#pragma once
#include <optional>

struct GpuMetrics {
    std::optional<double> usagePercentage;
    std::optional<double> usedVramMB;
    std::optional<double> temperatureC;
};
