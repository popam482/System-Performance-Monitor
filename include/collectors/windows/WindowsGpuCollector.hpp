#pragma once
#ifdef _WIN32

#include "collectors/IGpuCollector.hpp"

#include <dxgi.h>
#include <dxgi1_4.h>
#include <pdh.h>
#include <string>
#include <windows.h>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "dxgi.lib")

class WindowsGpuCollector : public IGpuCollector {
public:
    WindowsGpuCollector();
    ~WindowsGpuCollector() override;

    GpuData collectGpuData() override;

private:
    bool initializePdh();
    void cleanupPdh();
    bool readGpuInfo();
    double getGpuUsage();
    double getUsedVramMb();

    PDH_HQUERY gpuQuery = nullptr;
    PDH_HCOUNTER gpuUsageCounter = nullptr;
    bool pdhReady = false;

    UINT selectedAdapterIndex = 0;
    std::string gpuName = "Unknown GPU";
    double totalVramMB = 0.0;
    std::string driverVersion = "N/A";
};

#endif // _WIN32
