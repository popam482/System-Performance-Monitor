#pragma once
#include <optional>

struct RamMetrics{
    std::optional<unsigned long long> usedMemoryMB;
    std::optional<unsigned long long> availableMemoryMB;
    std::optional<double> usagePercentage;
};
