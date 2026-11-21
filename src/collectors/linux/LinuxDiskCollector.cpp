#include "collectors/linux/LinuxDiskCollector.hpp"

#include <cstdint>
#include <limits>
#include <system_error>
#include <utility>

LinuxDiskCollector::LinuxDiskCollector(std::filesystem::path path)
    : monitoredPath(std::move(path)) {}

DiskData linux_disk_detail::fromSpaceInfo(const std::filesystem::path& path,
                                         const std::filesystem::space_info& space) {
    DiskData data{};
    // Retain the existing field name: on Linux the display label is a path.
    data.info.driveLetter = path.string();
    constexpr auto unknown = std::numeric_limits<std::uintmax_t>::max();
    if (space.capacity == unknown || space.free == unknown ||
        space.capacity == 0 || space.free > space.capacity) return data;

    // free includes reserved blocks, matching the Windows collector's
    // totalFreeBytes. available would describe space usable by this user.
    const auto usedBytes = space.capacity - space.free;
    constexpr double bytesPerGiB = 1024.0 * 1024.0 * 1024.0;
    data.info.totalSpaceGB = static_cast<double>(space.capacity) / bytesPerGiB;
    data.metrics.freeSpaceGB = static_cast<double>(space.free) / bytesPerGiB;
    data.metrics.usedSpaceGB = static_cast<double>(usedBytes) / bytesPerGiB;
    data.metrics.usagePercentage = 100.0 * static_cast<double>(usedBytes) /
                                  static_cast<double>(space.capacity);
    return data;
}

DiskData LinuxDiskCollector::collectDiskData() {
    DiskData unavailable{};
    unavailable.info.driveLetter = monitoredPath.string();
    std::error_code error;
    // Some standard-library backends can resolve only the drive/root even for
    // a missing path. Require the selected path itself to exist.
    if (monitoredPath.empty() || !std::filesystem::exists(monitoredPath, error) || error)
        return unavailable;
    // On Linux, filesystem::space queries statvfs and uses f_frsize units.
    const auto space = std::filesystem::space(monitoredPath, error);
    if (error) {
        return unavailable; // No fallback to / or to stale measurements.
    }
    return linux_disk_detail::fromSpaceInfo(monitoredPath, space);
}
