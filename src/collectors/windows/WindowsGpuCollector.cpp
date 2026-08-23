#ifdef _WIN32

#include "collectors/windows/WindowsGpuCollector.hpp"
#include <iostream>
#include <vector>
#include <dxgi.h>
#include <dxgi1_4.h>

WindowsGpuCollector::WindowsGpuCollector() {
    readGpuInfo();
    initializePDH();
}

WindowsGpuCollector::~WindowsGpuCollector() {
    cleanupPDH();
}

void WindowsGpuCollector::readGpuInfo() {
    IDXGIFactory* pFactory = NULL;
    if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory))) {
        gpuName = "Unknown GPU";
        totalVramMB = 0.0;
        return;
    }

    IDXGIAdapter* pAdapter = NULL;
    // principal gpu adapter (index 0) - intel graphics
    if (pFactory->EnumAdapters(0, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC desc;
        pAdapter->GetDesc(&desc);

        std::wstring ws(desc.Description);
        gpuName = std::string(ws.begin(), ws.end());

        totalVramMB = static_cast<double>(desc.DedicatedVideoMemory) / (1024.0 * 1024.0);

        pAdapter->Release();
    }
    pFactory->Release();

    driverVersion = "N/A"; 
}

bool WindowsGpuCollector::initializePDH() {
    PDH_STATUS status = PdhOpenQuery(NULL, 0, &gpuQuery);
    if (status != ERROR_SUCCESS) return false;

    status = PdhAddCounter(gpuQuery, TEXT("\\GPU Engine(*_engtype_3D)\\% Utilization"), 0, &gpuUsageCounter);
    
    PdhCollectQueryData(gpuQuery);
    return true;
}

void WindowsGpuCollector::cleanupPDH() {
    if (gpuQuery) {
        PdhCloseQuery(gpuQuery);
        gpuQuery = NULL;
    }
}

double WindowsGpuCollector::getGpuUsage() {
    if (!gpuQuery || !gpuUsageCounter) return 0.0;

    PDH_STATUS status = PdhCollectQueryData(gpuQuery);
    if (status != ERROR_SUCCESS) return 0.0;

    DWORD bufferSize = 0;
    DWORD itemCount = 0;

    PdhGetFormattedCounterArray(gpuUsageCounter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, NULL);

    if (bufferSize == 0) return 0.0;

    std::vector<BYTE> buffer(bufferSize);
    PDH_FMT_COUNTERVALUE_ITEM* pItems = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(buffer.data());

    status = PdhGetFormattedCounterArray(gpuUsageCounter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, pItems);

    double maxUsage = 0.0;
    if (status == ERROR_SUCCESS) {
        for (DWORD i = 0; i < itemCount; ++i) {
            if (pItems[i].FmtValue.doubleValue > maxUsage) {
                maxUsage = pItems[i].FmtValue.doubleValue;
            }
        }
    }

    return maxUsage;
}

double WindowsGpuCollector::getUsedVram() {
    IDXGIFactory4* pFactory = NULL;
    double usedMB = 0.0;

    if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&pFactory))) {
        IDXGIAdapter3* pAdapter = NULL;
        if (pFactory->EnumAdapters1(0, reinterpret_cast<IDXGIAdapter1**>(&pAdapter)) != DXGI_ERROR_NOT_FOUND) {
            DXGI_QUERY_VIDEO_MEMORY_INFO info;
            if (SUCCEEDED(pAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
                usedMB = static_cast<double>(info.CurrentUsage) / (1024.0 * 1024.0);
            }
            pAdapter->Release();
        }
        pFactory->Release();
    }
    return usedMB;
}

GpuData WindowsGpuCollector::collectGpuData() {
    GpuData data;


    data.info.gpuName = gpuName;
    data.info.totalVramMB = totalVramMB;
    data.info.driverVersion = driverVersion;

    data.metrics.usagePercentage = getGpuUsage();
    data.metrics.usedVramMB = getUsedVram();
    data.metrics.temperatureC = 0.0;

    return data;
}

#endif // _WIN32