#pragma once

#include "collectors/IMemoryCollector.hpp"
#include <filesystem>

class LinuxMemoryCollector : public IMemoryCollector {
public:
    // Alternate proc roots support tests without reading host memory statistics.
    explicit LinuxMemoryCollector(std::filesystem::path procRoot = "/proc");
    RamData collectMemoryData() override;

private:
    std::filesystem::path meminfoPath;
};
