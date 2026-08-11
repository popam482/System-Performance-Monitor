#pragma once

#ifdef _WIN32

#include "../ICpuCollector.hpp"
#include <pdh.h>
#include <string>
#include <vector>

#pragma comment(lib, "pdh.lib")

class WindowsCpuCollector : public ICpuCollector {
public:
    WindowsCpuCollector();
    ~WindowsCpuCollector() override;

    CpuData collectCpuData() override;

private:
    PDH_HQUERY cpuQuery = NULL;
    PDH_HCOUNTER cpuTotalHandle = NULL;
    std::vector<PDH_HCOUNTER> perCoreHandles;
    
    bool initializePDH();
    void cleanupPDH();
    double getCpuUsage();
    double getCurrentFrequency();
    void readCpuInfo();
    
    std::string cpuName;
    unsigned int physicalCores = 0;
    unsigned int logicalThreads = 0;
    double baseFrequency = 0.0;
    double maxFrequency = 0.0;
};

#endif // _WIN32
