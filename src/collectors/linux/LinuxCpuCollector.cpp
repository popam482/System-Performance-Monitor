#include "collectors/linux/LinuxCpuCollector.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <limits>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

namespace {
std::string trim(const std::string& text) {
    const auto start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    return text.substr(start, text.find_last_not_of(" \t\r\n") - start + 1);
}

template <typename T>
std::optional<T> integer(const std::string& text) {
    T value{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) return std::nullopt;
    return value;
}

std::optional<long long> readInteger(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::string text;
    if (!std::getline(file, text)) return std::nullopt;
    return integer<long long>(trim(text));
}

std::optional<double> frequency(const std::filesystem::path& path) {
    const auto khz = readInteger(path);
    if (!khz || *khz <= 0) return std::nullopt;
    return static_cast<double>(*khz) / 1000000.0;
}

struct ProcCpuInfo {
    std::string name;
    std::map<unsigned int, double> mhz;
};

ProcCpuInfo readCpuInfo(const std::filesystem::path& path) {
    ProcCpuInfo info;
    std::ifstream file(path);
    std::string line, fallback;
    std::optional<unsigned int> cpu;
    while (std::getline(file, line)) {
        if (trim(line).empty()) { cpu.reset(); continue; }
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        const auto key = trim(line.substr(0, colon));
        const auto value = trim(line.substr(colon + 1));
        if (key == "processor") cpu = integer<unsigned int>(value);
        else if (key == "model name" && info.name.empty()) info.name = value;
        else if ((key == "Hardware" || key == "Processor") && fallback.empty()) fallback = value;
        else if (key == "cpu MHz" && cpu) {
            std::istringstream stream(value);
            stream.imbue(std::locale::classic());
            double mhz = 0;
            if (stream >> mhz && std::isfinite(mhz) && mhz > 0) {
                stream >> std::ws;
                if (stream.eof()) info.mhz[*cpu] = mhz;
            }
        }
    }
    if (info.name.empty()) info.name = fallback;
    return info;
}
}

LinuxCpuCollector::LinuxCpuCollector()
    : LinuxCpuCollector("/proc", "/sys/devices/system/cpu") {}

LinuxCpuCollector::LinuxCpuCollector(std::filesystem::path proc, std::filesystem::path sys)
    : procRoot(std::move(proc)), cpuRoot(std::move(sys)), previousTimes(readTimes()) {}

std::optional<LinuxCpuCollector::CpuTimes> LinuxCpuCollector::readTimes() const {
    std::ifstream file(procRoot / "stat");
    CpuTimes times;
    std::string line;
    bool aggregate = false;
    std::set<unsigned int> cpus;
    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string name;
        stream >> name;
        if (name == "cpu") {
            if (aggregate) return std::nullopt;
            aggregate = true;
            // user, nice, system, idle, iowait, irq, softirq and steal.
            for (std::size_t i = 0; i < times.ticks.size(); ++i) {
                std::string token;
                if (!(stream >> token)) {
                    if (i < 4) return std::nullopt;
                    break; // Older kernels may omit the final fields.
                }
                const auto value = integer<std::uint64_t>(token);
                if (!value) return std::nullopt;
                times.ticks[i] = *value;
            }
        } else if (name.size() > 3 && name.compare(0, 3, "cpu") == 0) {
            const auto id = integer<unsigned int>(name.substr(3));
            if (id && !cpus.insert(*id).second) return std::nullopt;
        }
    }
    if (!aggregate || cpus.empty()) return std::nullopt;
    times.onlineCpus.assign(cpus.begin(), cpus.end());
    return times;
}

std::optional<double> LinuxCpuCollector::readUsage(const std::optional<CpuTimes>& current) {
    const auto previous = std::exchange(previousTimes, current);
    if (!current || !previous || current->onlineCpus != previous->onlineCpus) return std::nullopt;
    std::uint64_t total = 0, idle = 0;
    for (std::size_t i = 0; i < current->ticks.size(); ++i) {
        // Counter resets and decreasing iowait must not wrap unsigned deltas.
        if (current->ticks[i] < previous->ticks[i]) return std::nullopt;
        const auto delta = current->ticks[i] - previous->ticks[i];
        if (delta > std::numeric_limits<std::uint64_t>::max() - total) return std::nullopt;
        total += delta;
        if (i == 3 || i == 4) idle += delta;
    }
    if (total == 0) return std::nullopt;
    return 100.0 * static_cast<double>(total - idle) / static_cast<double>(total);
}

CpuData LinuxCpuCollector::collectCpuData() {
    CpuData data{};
    const auto info = readCpuInfo(procRoot / "cpuinfo");
    data.info.cpuName = info.name;
    const auto times = readTimes();
    data.metrics.usage = readUsage(times);
    if (!times || times->onlineCpus.size() > std::numeric_limits<unsigned int>::max()) return data;
    data.info.logicalThreads = static_cast<unsigned int>(times->onlineCpus.size());

    std::set<std::tuple<long long, long long, long long>> cores;
    bool topologyComplete = true, baseComplete = true, maxComplete = true, currentComplete = true;
    std::optional<double> base;
    double maximum = 0.0, currentSum = 0.0;
    for (const auto cpu : times->onlineCpus) {
        const auto root = cpuRoot / ("cpu" + std::to_string(cpu));
        const auto package = readInteger(root / "topology/physical_package_id");
        const auto core = readInteger(root / "topology/core_id");
        const auto die = readInteger(root / "topology/die_id");
        if (!package || *package < 0 || !core || *core < 0) topologyComplete = false;
        else cores.emplace(*package, die && *die >= 0 ? *die : 0, *core);

        const auto cpuBase = frequency(root / "cpufreq/base_frequency");
        if (!cpuBase || (base && *base != *cpuBase)) baseComplete = false;
        if (cpuBase) base = cpuBase;
        const auto cpuMax = frequency(root / "cpufreq/cpuinfo_max_freq");
        if (!cpuMax) maxComplete = false;
        else maximum = std::max(maximum, *cpuMax);
        auto current = frequency(root / "cpufreq/cpuinfo_cur_freq");
        if (!current) current = frequency(root / "cpufreq/scaling_cur_freq");
        if (!current) {
            const auto mhz = info.mhz.find(cpu);
            if (mhz != info.mhz.end()) current = mhz->second / 1000.0;
        }
        if (!current) currentComplete = false;
        else currentSum += *current;
    }
    if (topologyComplete) data.info.physicalCores = static_cast<unsigned int>(cores.size());
    if (baseComplete) data.info.baseFrequencyGHz = base;
    if (maxComplete) data.info.maxFrequencyGHz = maximum;
    if (currentComplete) data.metrics.currentFrequency = currentSum / times->onlineCpus.size();
    return data;
}
