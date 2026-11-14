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
    std::optional<double> getGpuUsage();
    std::optional<double> getUsedVramMb();

    PDH_HQUERY gpuQuery = nullptr;
    PDH_HCOUNTER gpuUsageCounter = nullptr;
    PDH_HCOUNTER gpuMemoryCounter = nullptr;
    bool pdhReady = false;

    std::string adapterInstance;
    std::string gpuName;
    std::optional<double> totalVramMB;
    std::optional<std::string> driverVersion;
};

#endif // _WIN32
