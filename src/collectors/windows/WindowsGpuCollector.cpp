#ifdef _WIN32
#include "collectors/windows/WindowsGpuCollector.hpp"
#include <pdhmsg.h>
#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <vector>

namespace {
std::string wideToUtf8(const wchar_t* wide) {
    if (!wide) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return {};
    std::string result(static_cast<size_t>(size), '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), size, nullptr, nullptr)) return {};
    result.pop_back();
    return result;
}
bool validStatus(DWORD status) {
    return status == PDH_CSTATUS_VALID_DATA || status == PDH_CSTATUS_NEW_DATA;
}
std::string luidInstance(LUID luid) {
    std::ostringstream text;
    text << "luid_0x" << std::hex << std::setfill('0') << std::setw(8)
         << static_cast<DWORD>(luid.HighPart) << "_0x" << std::setw(8) << luid.LowPart;
    return text.str();
}
std::optional<std::string> readDriverVersion(IDXGIAdapter1* adapter) {
    LARGE_INTEGER version{};
    if (FAILED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &version))) return std::nullopt;
    std::ostringstream text;
    text << HIWORD(version.HighPart) << '.' << LOWORD(version.HighPart) << '.'
         << HIWORD(version.LowPart) << '.' << LOWORD(version.LowPart);
    return text.str();
}
}

WindowsGpuCollector::WindowsGpuCollector() {
    if (readGpuInfo()) initializePdh();
}
WindowsGpuCollector::~WindowsGpuCollector() { cleanupPdh(); }

bool WindowsGpuCollector::readGpuInfo() {
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)))) return false;
    SIZE_T bestVram = 0;
    bool found = false;
    for (UINT index = 0;; ++index) {
        IDXGIAdapter1* adapter = nullptr;
        const HRESULT status = factory->EnumAdapters1(index, &adapter);
        if (FAILED(status)) break;
        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
                (!found || desc.DedicatedVideoMemory > bestVram)) {
            found = true;
            bestVram = desc.DedicatedVideoMemory;
            gpuName = wideToUtf8(desc.Description);
            totalVramMB = static_cast<double>(bestVram) / (1024.0 * 1024.0);
            driverVersion = readDriverVersion(adapter);
            adapterInstance = luidInstance(desc.AdapterLuid);
        }
        adapter->Release();
    }
    factory->Release();
    return found;
}

bool WindowsGpuCollector::initializePdh() {
    cleanupPdh();
    if (PdhOpenQueryA(nullptr, 0, &gpuQuery) != ERROR_SUCCESS) return false;
    // Limit both counters to the selected DXGI adapter, identified by LUID.
    // PDH wildcard instances are enumerated first, then filtered by LUID/type.
    const std::string usagePath = "\\GPU Engine(*)\\% Utilization";
    if (PdhAddEnglishCounterA(gpuQuery, usagePath.c_str(), 0, &gpuUsageCounter) != ERROR_SUCCESS)
        gpuUsageCounter = nullptr;
    const std::string memoryPath = "\\GPU Adapter Memory(" + adapterInstance + "_phys_0)\\Dedicated Usage";
    if (PdhAddEnglishCounterA(gpuQuery, memoryPath.c_str(), 0, &gpuMemoryCounter) != ERROR_SUCCESS)
        gpuMemoryCounter = nullptr;
    if ((!gpuUsageCounter && !gpuMemoryCounter) || PdhCollectQueryData(gpuQuery) != ERROR_SUCCESS) {
        cleanupPdh();
        return false;
    }
    pdhReady = true;
    return true;
}
void WindowsGpuCollector::cleanupPdh() {
    if (gpuQuery) PdhCloseQuery(gpuQuery);
    gpuQuery = nullptr;
    gpuUsageCounter = nullptr;
    gpuMemoryCounter = nullptr;
    pdhReady = false;
}
std::optional<double> WindowsGpuCollector::getGpuUsage() {
    if (!gpuUsageCounter) return std::nullopt;
    DWORD size = 0, count = 0;
    PDH_STATUS status = PdhGetFormattedCounterArrayA(gpuUsageCounter, PDH_FMT_DOUBLE,
                                                  &size, &count, nullptr);
    if ((status != PDH_MORE_DATA && status != ERROR_SUCCESS) || size == 0) return std::nullopt;
    std::vector<BYTE> buffer(size);
    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_A*>(buffer.data());
    if (PdhGetFormattedCounterArrayA(gpuUsageCounter, PDH_FMT_DOUBLE, &size, &count, items)
            != ERROR_SUCCESS) return std::nullopt;
    // Instances are per process. Sum contributions on each physical engine,
    // then report the busiest 3D engine of the selected adapter.
    std::map<std::string, double> engines;
    for (DWORD i = 0; i < count; ++i) {
        if (!validStatus(items[i].FmtValue.CStatus) || !items[i].szName) continue;
        const std::string name(items[i].szName);
        if (name.find(adapterInstance + "_") == std::string::npos) continue;
        if (name.find("engtype_3D") == std::string::npos) continue;
        const auto physical = name.find("phys_");
        if (physical == std::string::npos) continue;
        engines[name.substr(physical)] += items[i].FmtValue.doubleValue;
    }
    if (engines.empty()) return std::nullopt;
    double usage = 0.0;
    for (const auto& engine : engines) usage = (std::max)(usage, engine.second);
    return std::clamp(usage, 0.0, 100.0);
}
std::optional<double> WindowsGpuCollector::getUsedVramMb() {
    if (!gpuMemoryCounter) return std::nullopt;
    PDH_FMT_COUNTERVALUE value{};
    if (PdhGetFormattedCounterValue(gpuMemoryCounter, PDH_FMT_LARGE, nullptr, &value)
            != ERROR_SUCCESS || !validStatus(value.CStatus) || value.largeValue < 0) return std::nullopt;
    return static_cast<double>(value.largeValue) / (1024.0 * 1024.0);
}
GpuData WindowsGpuCollector::collectGpuData() {
    GpuData data{};
    data.info.gpuName = gpuName;
    data.info.totalVramMB = totalVramMB;
    data.info.driverVersion = driverVersion;
    if (pdhReady && PdhCollectQueryData(gpuQuery) == ERROR_SUCCESS) {
        data.metrics.usagePercentage = getGpuUsage();
        data.metrics.usedVramMB = getUsedVramMb();
    }
    // DXGI/PDH do not expose a portable temperature sensor. Keep it absent.
    return data;
}
#endif
