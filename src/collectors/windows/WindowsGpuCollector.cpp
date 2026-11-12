#ifdef _WIN32

#include "collectors/windows/WindowsGpuCollector.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

namespace {
std::string wideToUtf8(const wchar_t* wide) {
    if (!wide) return {};
    const int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 1) return {};
    std::string result(static_cast<size_t>(sizeNeeded - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), sizeNeeded, nullptr, nullptr);
    return result;
}

bool isSoftwareAdapter(const DXGI_ADAPTER_DESC1& desc) {
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
        return true;
    }
    const std::wstring name(desc.Description);
    return name.find(L"Microsoft Basic Render Driver") != std::wstring::npos;
}
} // namespace

WindowsGpuCollector::WindowsGpuCollector() {
    readGpuInfo();
    initializePdh();
}

WindowsGpuCollector::~WindowsGpuCollector() {
    cleanupPdh();
}

bool WindowsGpuCollector::readGpuInfo() {
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)))) {
        gpuName = "Unknown GPU";
        totalVramMB = 0.0;
        return false;
    }

    IDXGIAdapter1* adapter = nullptr;
    UINT bestIndex = 0;
    SIZE_T bestVram = 0;
    bool foundHardware = false;
    DXGI_ADAPTER_DESC1 bestDesc{};

    for (UINT index = 0; factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND; ++index) {
        DXGI_ADAPTER_DESC1 desc{};
        if (FAILED(adapter->GetDesc1(&desc))) {
            adapter->Release();
            continue;
        }

        if (!isSoftwareAdapter(desc)) {
            if (!foundHardware || desc.DedicatedVideoMemory > bestVram) {
                bestIndex = index;
                bestVram = desc.DedicatedVideoMemory;
                bestDesc = desc;
                foundHardware = true;
            }
        }

        adapter->Release();
    }

    // Fallback to adapter 0 if only software adapters exist.
    if (!foundHardware) {
        if (SUCCEEDED(factory->EnumAdapters1(0, &adapter))) {
            adapter->GetDesc1(&bestDesc);
            bestIndex = 0;
            bestVram = bestDesc.DedicatedVideoMemory;
            adapter->Release();
            foundHardware = true;
        }
    }

    factory->Release();

    if (!foundHardware) {
        gpuName = "Unknown GPU";
        totalVramMB = 0.0;
        return false;
    }

    selectedAdapterIndex = bestIndex;
    gpuName = wideToUtf8(bestDesc.Description);
    if (gpuName.empty()) {
        gpuName = "Unknown GPU";
    }
    totalVramMB = static_cast<double>(bestVram) / (1024.0 * 1024.0);
    driverVersion = "N/A";
    return true;
}

bool WindowsGpuCollector::initializePdh() {
    cleanupPdh();

    PDH_STATUS status = PdhOpenQueryA(nullptr, 0, &gpuQuery);
    if (status != ERROR_SUCCESS) {
        gpuQuery = nullptr;
        pdhReady = false;
        return false;
    }

    // Prefer English counter API so localized Windows still resolves the path.
    status = PdhAddEnglishCounterA(
        gpuQuery,
        "\\GPU Engine(*_engtype_3D)\\% Utilization",
        0,
        &gpuUsageCounter
    );

    // Fallback for toolchains / systems without PdhAddEnglishCounterA.
    if (status != ERROR_SUCCESS) {
        status = PdhAddCounterA(
            gpuQuery,
            "\\GPU Engine(*_engtype_3D)\\% Utilization",
            0,
            &gpuUsageCounter
        );
    }

    if (status != ERROR_SUCCESS) {
        cleanupPdh();
        pdhReady = false;
        return false;
    }

    // Prime the counter; first real sample needs a later Collect call.
    status = PdhCollectQueryData(gpuQuery);
    pdhReady = (status == ERROR_SUCCESS);
    if (!pdhReady) {
        cleanupPdh();
    }
    return pdhReady;
}

void WindowsGpuCollector::cleanupPdh() {
    if (gpuQuery) {
        PdhCloseQuery(gpuQuery);
    }
    gpuQuery = nullptr;
    gpuUsageCounter = nullptr;
    pdhReady = false;
}

double WindowsGpuCollector::getGpuUsage() {
    if (!pdhReady || !gpuQuery || !gpuUsageCounter) {
        return 0.0;
    }

    PDH_STATUS status = PdhCollectQueryData(gpuQuery);
    if (status != ERROR_SUCCESS) {
        return 0.0;
    }

    DWORD bufferSize = 0;
    DWORD itemCount = 0;
    status = PdhGetFormattedCounterArrayA(
        gpuUsageCounter,
        PDH_FMT_DOUBLE,
        &bufferSize,
        &itemCount,
        nullptr
    );

    // PDH_MORE_DATA is expected on the size probe.
    if (status != PDH_MORE_DATA && status != ERROR_SUCCESS) {
        return 0.0;
    }
    if (bufferSize == 0) {
        return 0.0;
    }

    std::vector<BYTE> buffer(bufferSize);
    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_A*>(buffer.data());

    status = PdhGetFormattedCounterArrayA(
        gpuUsageCounter,
        PDH_FMT_DOUBLE,
        &bufferSize,
        &itemCount,
        items
    );

    if (status != ERROR_SUCCESS) {
        return 0.0;
    }

    double maxUsage = 0.0;
    for (DWORD i = 0; i < itemCount; ++i) {
        if (items[i].FmtValue.CStatus != ERROR_SUCCESS) {
            continue;
        }
        maxUsage = (std::max)(maxUsage, items[i].FmtValue.doubleValue);
    }

    if (maxUsage < 0.0) maxUsage = 0.0;
    if (maxUsage > 100.0) maxUsage = 100.0;
    return maxUsage;
}

double WindowsGpuCollector::getUsedVramMb() {
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)))) {
        return 0.0;
    }

    IDXGIAdapter1* adapter1 = nullptr;
    if (factory->EnumAdapters1(selectedAdapterIndex, &adapter1) == DXGI_ERROR_NOT_FOUND) {
        factory->Release();
        return 0.0;
    }

    double usedMb = 0.0;
    IDXGIAdapter3* adapter3 = nullptr;
    if (SUCCEEDED(adapter1->QueryInterface(__uuidof(IDXGIAdapter3), reinterpret_cast<void**>(&adapter3)))) {
        DXGI_QUERY_VIDEO_MEMORY_INFO info{};
        if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
            usedMb = static_cast<double>(info.CurrentUsage) / (1024.0 * 1024.0);
        }
        adapter3->Release();
    }

    adapter1->Release();
    factory->Release();
    return usedMb;
}

GpuData WindowsGpuCollector::collectGpuData() {
    GpuData data;
    data.info.gpuName = gpuName;
    data.info.totalVramMB = totalVramMB;
    data.info.driverVersion = driverVersion;

    data.metrics.usagePercentage = getGpuUsage();
    data.metrics.usedVramMB = getUsedVramMb();
    data.metrics.temperatureC = 0.0; // needs vendor APIs (NVAPI/ADL); left for later
    return data;
}

#endif // _WIN32
