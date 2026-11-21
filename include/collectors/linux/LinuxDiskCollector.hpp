#pragma once

#include "collectors/IDiskCollector.hpp"
#include <filesystem>

// Space on the mounted filesystem containing path (not physical disk I/O).
class LinuxDiskCollector : public IDiskCollector {
public:
    explicit LinuxDiskCollector(std::filesystem::path path = "/");
    DiskData collectDiskData() override;

private:
    std::filesystem::path monitoredPath;
};

namespace linux_disk_detail {
// Shared conversion keeps invalid readings and byte arithmetic testable.
DiskData fromSpaceInfo(const std::filesystem::path& path, const std::filesystem::space_info& space);
}
