#pragma once
#include <string>
#include <optional>

struct GpuInfo{
    std::string gpuName;
    std::optional<double> totalVramMB;
    std::optional<std::string> driverVersion;
};
