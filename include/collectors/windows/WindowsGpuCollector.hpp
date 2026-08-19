#pragma once
#ifdef _WIN32

#include "collectors/IGpuCollector.hpp"
#include <pdh.h>
#include <windows.h>
#include <string>

class WindowsGpuCollector : public IGpuCollector {
private:
    PDH_HQUERY gpuQuery = NULL;
    PDH_HCOUNTER gpuUsageCounter = NULL;

    std::string gpuName;
    double totalVramMB = 0.0;
    std::string driverVersion;

    bool initializePDH();
    void cleanupPDH();
    void readGpuInfo();

    double getGpuUsage();
    double getUsedVram();

public:
    WindowsGpuCollector();
    ~WindowsGpuCollector() override;

    GpuData collectGpuData() override;

};
#endif // _WIN32