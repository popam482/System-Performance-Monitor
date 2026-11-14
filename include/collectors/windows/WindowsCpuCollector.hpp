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
    
    bool initializePDH();
    void cleanupPDH();
    std::optional<double> getCpuUsage();
    void readFrequencies(CpuData& data);
    void readCpuInfo();
    
    std::string cpuName;
    std::optional<unsigned int> physicalCores;
    std::optional<unsigned int> logicalThreads;
    std::optional<double> baseFrequency;
};

#endif // _WIN32
