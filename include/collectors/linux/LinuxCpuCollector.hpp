#pragma once

#include "collectors/ICpuCollector.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

class LinuxCpuCollector : public ICpuCollector {
public:
    LinuxCpuCollector();
    LinuxCpuCollector(std::filesystem::path procRoot, std::filesystem::path cpuRoot);
    CpuData collectCpuData() override;

private:
    struct CpuTimes {
        std::array<std::uint64_t, 8> ticks{};
        std::vector<unsigned int> onlineCpus;
    };
    std::filesystem::path procRoot;
    std::filesystem::path cpuRoot;
    std::optional<CpuTimes> previousTimes;

    std::optional<CpuTimes> readTimes() const;
    std::optional<double> readUsage(const std::optional<CpuTimes>& current);
};
