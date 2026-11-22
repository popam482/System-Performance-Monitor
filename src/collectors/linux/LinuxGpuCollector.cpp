#include "collectors/linux/LinuxGpuCollector.hpp"
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef __linux__
#include <dlfcn.h>
#endif

namespace {
namespace fs = std::filesystem;
constexpr double bytesPerMiB = 1024.0 * 1024.0;

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}
std::string readText(const fs::path& path) {
    std::ifstream file(path);
    std::string line;
    return std::getline(file, line) ? trim(line) : std::string{};
}
template <typename T>
std::optional<T> readNumber(const fs::path& path, int base = 10) {
    auto text = readText(path);
    if (base == 16 && text.compare(0, 2, "0x") == 0) text.erase(0, 2);
    T value{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return std::nullopt;
    return value;
}
std::vector<fs::path> children(const fs::path& path) {
    std::vector<fs::path> result;
    std::error_code error;
    fs::directory_iterator it(path, error), end;
    while (!error && it != end) { result.push_back(it->path()); it.increment(error); }
    std::sort(result.begin(), result.end());
    return result;
}
std::string hexId(unsigned int value) {
    std::ostringstream text;
    text << std::hex << std::setfill('0') << std::setw(4) << value;
    return text.str();
}
std::string pciName(unsigned int vendor, unsigned int device) {
    for (const auto* database : {"/usr/share/hwdata/pci.ids", "/usr/share/misc/pci.ids", "/usr/share/pci.ids"}) {
        std::ifstream file(database);
        std::string line, vendorName;
        bool inVendor = false;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            if (line[0] != '\t') {
                if (inVendor) break;
                inVendor = line.size() > 4 && line.substr(0, 4) == hexId(vendor);
                if (inVendor) vendorName = trim(line.substr(4));
            } else if (inVendor && line.size() > 5 && line[1] != '\t' && line.substr(1, 4) == hexId(device)) {
                return vendorName + " " + trim(line.substr(5));
            }
        }
    }
    const char* name = vendor == 0x10de ? "NVIDIA" : vendor == 0x1002 ? "AMD" : vendor == 0x8086 ? "Intel" : "PCI GPU";
    return std::string(name) + " [" + hexId(vendor) + ":" + hexId(device) + "]";
}

// Minimal ABI declarations for the stable NVML v1 memory/utilization queries.
// The driver library is optional; CUDA/NVML development headers are not required.
struct NvmlDevice;
using Device = NvmlDevice*;
struct Memory { unsigned long long total, free, used; };
struct Utilization { unsigned int gpu, memory; };
static_assert(sizeof(unsigned long long) == 8 && sizeof(unsigned int) == 4,
              "NVML requires 64-bit memory counters and 32-bit unsigned integers");

class Nvml {
public:
    Nvml() {
#ifdef __linux__
        library = dlopen("libnvidia-ml.so.1", RTLD_LAZY | RTLD_LOCAL);
        if (!library) return;
        const auto init = symbol<int (*)()>("nvmlInit_v2");
        shutdown = symbol<int (*)()>("nvmlShutdown");
        count = symbol<int (*)(unsigned int*)>("nvmlDeviceGetCount_v2");
        byIndex = symbol<int (*)(unsigned int, Device*)>("nvmlDeviceGetHandleByIndex_v2");
        byPci = symbol<int (*)(const char*, Device*)>("nvmlDeviceGetHandleByPciBusId_v2");
        name = symbol<int (*)(Device, char*, unsigned int)>("nvmlDeviceGetName");
        driver = symbol<int (*)(char*, unsigned int)>("nvmlSystemGetDriverVersion");
        memory = symbol<int (*)(Device, Memory*)>("nvmlDeviceGetMemoryInfo");
        utilization = symbol<int (*)(Device, Utilization*)>("nvmlDeviceGetUtilizationRates");
        temperature = symbol<int (*)(Device, int, unsigned int*)>("nvmlDeviceGetTemperature");
        ready = init && shutdown && count && byIndex && init() == 0;
#endif
    }
    ~Nvml() {
        if (ready) shutdown();
#ifdef __linux__
        if (library) dlclose(library);
#endif
    }
    Nvml(const Nvml&) = delete;
    Nvml& operator=(const Nvml&) = delete;

    std::optional<Memory> readMemory(Device device) const {
        Memory info{};
        constexpr auto unavailable = std::numeric_limits<unsigned long long>::max();
        if (!device || !memory || memory(device, &info) != 0 || info.total == unavailable ||
            info.used > info.total || info.free > info.total) return std::nullopt;
        return info;
    }
    bool ready = false;
    int (*count)(unsigned int*) = nullptr;
    int (*byIndex)(unsigned int, Device*) = nullptr;
    int (*byPci)(const char*, Device*) = nullptr;
    int (*name)(Device, char*, unsigned int) = nullptr;
    int (*driver)(char*, unsigned int) = nullptr;
    int (*memory)(Device, Memory*) = nullptr;
    int (*utilization)(Device, Utilization*) = nullptr;
    int (*temperature)(Device, int, unsigned int*) = nullptr;
private:
    template <typename T> T symbol(const char* name) {
#ifdef __linux__
        return reinterpret_cast<T>(dlsym(library, name));
#else
        (void)name;
        return nullptr;
#endif
    }
    void* library = nullptr;
    int (*shutdown)() = nullptr;
};

struct Adapter {
    Device nvmlDevice = nullptr;
    fs::path devicePath;
    GpuInfo info;
    bool bootDisplay = false;
};

std::optional<double> readTemperature(const fs::path& device) {
    for (const auto& sensor : children(device / "hwmon")) {
        const auto fault = readNumber<int>(sensor / "temp1_fault");
        if (fault && *fault != 0) continue;
        // temp1 is the edge/package sensor; do not substitute hotspot/VRAM temps.
        if (const auto value = readNumber<long long>(sensor / "temp1_input"))
            return static_cast<double>(*value) / 1000.0;
    }
    return std::nullopt;
}

void readSysfs(const fs::path& device, GpuData& data) {
    if (device.empty()) return;
    const auto total = readNumber<unsigned long long>(device / "mem_info_vram_total");
    const auto used = readNumber<unsigned long long>(device / "mem_info_vram_used");
    if (!data.info.totalVramMB && total) data.info.totalVramMB = static_cast<double>(*total) / bytesPerMiB;
    if (!data.metrics.usedVramMB && used && total && *used <= *total)
        data.metrics.usedVramMB = static_cast<double>(*used) / bytesPerMiB;
    if (!data.metrics.usagePercentage) {
        const auto usage = readNumber<unsigned int>(device / "gpu_busy_percent");
        if (usage && *usage <= 100) data.metrics.usagePercentage = *usage;
    }
    if (!data.metrics.temperatureC) data.metrics.temperatureC = readTemperature(device);
}
}

struct LinuxGpuCollector::Impl {
    Nvml nvml;
    std::optional<Adapter> selected;

    Impl() {
        std::vector<Adapter> adapters;
        unsigned int count = 0;
        if (nvml.ready && nvml.count(&count) == 0) {
            for (unsigned int i = 0; i < count; ++i) {
                Adapter adapter;
                if (nvml.byIndex(i, &adapter.nvmlDevice) != 0 || !adapter.nvmlDevice) continue;
                char name[128]{};
                if (nvml.name && nvml.name(adapter.nvmlDevice, name, sizeof(name)) == 0 && name[0])
                    adapter.info.gpuName = name;
                if (const auto memory = nvml.readMemory(adapter.nvmlDevice))
                    adapter.info.totalVramMB = static_cast<double>(memory->total) / bytesPerMiB;
                char version[128]{};
                if (nvml.driver && nvml.driver(version, sizeof(version)) == 0 && version[0])
                    adapter.info.driverVersion = version;
                adapters.push_back(adapter);
            }
        }
        std::set<fs::path> seen;
        for (const auto& card : children("/sys/class/drm")) {
            const auto id = card.filename().string();
            if (id.size() <= 4 || id.compare(0, 4, "card") != 0 ||
                id.find_first_not_of("0123456789", 4) != std::string::npos) continue;
            std::error_code error;
            const auto device = fs::canonical(card / "device", error);
            if (error || !seen.insert(device).second) continue;
            // Match the very same NVIDIA adapter by PCI address, never by card index.
            Device handle = nullptr;
            if (nvml.ready && nvml.byPci &&
                nvml.byPci(device.filename().string().c_str(), &handle) != 0) handle = nullptr;
            auto match = std::find_if(adapters.begin(), adapters.end(), [handle](const Adapter& adapter) {
                return handle && adapter.nvmlDevice == handle;
            });
            Adapter adapter;
            adapter.devicePath = device;
            adapter.bootDisplay = readNumber<int>(device / "boot_vga") == 1;
            if (match != adapters.end()) {
                match->devicePath = device;
                match->bootDisplay = adapter.bootDisplay;
                continue;
            }
            const auto vendor = readNumber<unsigned int>(device / "vendor", 16);
            const auto product = readNumber<unsigned int>(device / "device", 16);
            adapter.info.gpuName = readText(device / "product_name");
            if (adapter.info.gpuName.empty() && vendor && product)
                adapter.info.gpuName = pciName(*vendor, *product);
            const auto version = readText(device / "driver/module/version");
            if (!version.empty()) adapter.info.driverVersion = version;
            GpuData data{};
            readSysfs(device, data);
            adapter.info.totalVramMB = data.info.totalVramMB;
            adapters.push_back(adapter);
        }
        // Same policy as Windows: prefer the adapter with most reported VRAM.
        // With no VRAM information, prefer the boot display; ties stay stable.
        for (const auto& adapter : adapters) {
            if (!selected || adapter.info.totalVramMB.value_or(0) > selected->info.totalVramMB.value_or(0) ||
                (adapter.info.totalVramMB.value_or(0) == selected->info.totalVramMB.value_or(0) &&
                 adapter.bootDisplay && !selected->bootDisplay)) selected = adapter;
        }
    }

    GpuData collect() {
        GpuData data{};
        if (!selected) return data;
        data.info = selected->info;
        const auto device = selected->nvmlDevice;
        if (device) {
            if (const auto memory = nvml.readMemory(device)) {
                data.info.totalVramMB = static_cast<double>(memory->total) / bytesPerMiB;
                data.metrics.usedVramMB = static_cast<double>(memory->used) / bytesPerMiB;
            }
            Utilization usage{};
            if (nvml.utilization && nvml.utilization(device, &usage) == 0 && usage.gpu <= 100)
                data.metrics.usagePercentage = usage.gpu;
            unsigned int temperature = 0;
            if (nvml.temperature && nvml.temperature(device, 0, &temperature) == 0 &&
                temperature != std::numeric_limits<unsigned int>::max()) data.metrics.temperatureC = temperature;
        }
        readSysfs(selected->devicePath, data);
        return data;
    }
};

LinuxGpuCollector::LinuxGpuCollector() : impl(std::make_unique<Impl>()) {}
LinuxGpuCollector::~LinuxGpuCollector() = default;
GpuData LinuxGpuCollector::collectGpuData() { return impl->collect(); }
