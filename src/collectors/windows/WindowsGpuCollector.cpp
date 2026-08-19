#ifdef _WIN32

#include "collectors/windows/WindowsGpuCollector.hpp"
#include <iostream>

WindowsGpuCollector::WindowsGpuCollector() {
    readGpuInfo();
    initializePDH();
}

WindowsGpuCollector::~WindowsGpuCollector() {
    cleanupPDH();
}

void WindowsGpuCollector::readGpuInfo() {
    // read GPU info from registry 
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinSAT", 
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
    }

    gpuName = "NVIDIA / AMD Graphics Card";
    totalVramMB = 16000.0; // mock value
    driverVersion = "551.23";
}

bool WindowsGpuCollector::initializePDH() {
    PDH_STATUS status = PdhOpenQuery(NULL, 0, &gpuQuery);
    if (status != ERROR_SUCCESS) return false;

    status = PdhAddCounter(gpuQuery, TEXT("\\GPU Engine(*_engtype_3D)\\% Utilization"), 0, &gpuUsageCounter);
    if (status != ERROR_SUCCESS) {
        PdhCloseQuery(gpuQuery);
        gpuQuery = NULL;
        return false;
    }

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

    PDH_FMT_COUNTERVALUE value;
    status = PdhGetFormattedCounterValue(gpuUsageCounter, PDH_FMT_DOUBLE, NULL, &value);

    return (status == ERROR_SUCCESS) ? value.doubleValue : 0.0;
}

double WindowsGpuCollector::getUsedVram() {
    return 0.0; 
}

GpuData WindowsGpuCollector::collectGpuData() {
    GpuData data;

    // Static Info
    data.info.gpuName = gpuName;
    data.info.totalVramMB = totalVramMB;
    data.info.driverVersion = driverVersion;

    // Dynamic Metrics
    data.metrics.usagePercentage = getGpuUsage();
    data.metrics.usedVramMB = getUsedVram();
    data.metrics.temperatureC = 0.0;

    return data;
}

#endif // _WIN32