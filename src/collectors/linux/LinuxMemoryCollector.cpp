#include "collectors/linux/LinuxMemoryCollector.hpp"

#include <charconv>
#include <fstream>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>

namespace {
using MemorySize = unsigned long long;

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

std::optional<MemorySize> parseKiB(const std::string& text) {
    std::istringstream stream(text);
    stream.imbue(std::locale::classic());
    std::string number, unit, extra;
    if (!(stream >> number >> unit) || unit != "kB" || stream >> extra) return std::nullopt;
    MemorySize value = 0;
    const auto result = std::from_chars(number.data(), number.data() + number.size(), value);
    if (result.ec != std::errc{} || result.ptr != number.data() + number.size()) return std::nullopt;
    // Linux meminfo labels binary KiB as kB. Reject byte-conversion overflow.
    if (value > std::numeric_limits<MemorySize>::max() / 1024) return std::nullopt;
    return value;
}
}

LinuxMemoryCollector::LinuxMemoryCollector(std::filesystem::path procRoot)
    : meminfoPath(procRoot / "meminfo") {}

RamData LinuxMemoryCollector::collectMemoryData() {
    RamData data{};
    std::ifstream file(meminfoPath);
    if (!file) return data;

    std::optional<MemorySize> totalKiB, availableKiB;
    bool seenTotal = false, seenAvailable = false;
    std::string line;
    while (std::getline(file, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        const auto key = trim(line.substr(0, colon));
        if (key == "MemTotal") {
            // Duplicate relevant keys are ambiguous, even if one is valid.
            if (seenTotal) { totalKiB.reset(); continue; }
            seenTotal = true;
            totalKiB = parseKiB(line.substr(colon + 1));
        } else if (key == "MemAvailable") {
            if (seenAvailable) { availableKiB.reset(); continue; }
            seenAvailable = true;
            availableKiB = parseKiB(line.substr(colon + 1));
        }
    }
    if (file.bad() || !totalKiB || *totalKiB == 0) return data;
    data.info.totalMemoryBytes = *totalKiB * 1024;
    // MemFree excludes reclaimable cache; do not use it as MemAvailable.
    if (!availableKiB || *availableKiB > *totalKiB) return data;
    const auto usedKiB = *totalKiB - *availableKiB;
    data.metrics.availableMemoryMB = *availableKiB / 1024;
    data.metrics.usedMemoryMB = usedKiB / 1024;
    data.metrics.usagePercentage = 100.0 * static_cast<double>(usedKiB) / static_cast<double>(*totalKiB);
    // /proc/meminfo does not identify DDR technology. ramType remains absent.
    return data;
}
