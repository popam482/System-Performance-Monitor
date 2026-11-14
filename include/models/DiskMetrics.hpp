#pragma once
#include <optional>

struct DiskMetrics {
    std::optional<double> freeSpaceGB;
    std::optional<double> usedSpaceGB;
    std::optional<double> usagePercentage;
};
