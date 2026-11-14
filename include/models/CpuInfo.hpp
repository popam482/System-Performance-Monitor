#pragma once
#include <string>
#include <optional>

struct CpuInfo{
    std::string cpuName;
    std::optional<unsigned int> physicalCores;
    std::optional<unsigned int> logicalThreads;

    // Empty when the platform cannot report a nominal/base frequency.
    std::optional<double> baseFrequencyGHz;
    // Maximum specified frequency reported by the OS, not a turbo estimate.
    std::optional<double> maxFrequencyGHz;
};
