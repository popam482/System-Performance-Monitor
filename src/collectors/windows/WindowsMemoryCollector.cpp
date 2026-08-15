#ifdef _WIN32

#include "collectors/windows/WindowsMemoryCollector.hpp"
#include <iostream>

WindowsMemoryCollector::WindowsMemoryCollector() {
    readMemoryInfo();
}

void WindowsMemoryCollector::readMemoryInfo() {
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (!GlobalMemoryStatusEx(&memStatus)) {
        std::cerr << "Failed to retrieve system memory status." << std::endl;
        totalMemoryMB = 0;
        ramType = "Unknown";
        return;
    }
    
    // Convert bytes to MB
    totalMemoryMB = memStatus.ullTotalPhys / (1024 * 1024);
    ramType = "DDR4/DDR5"; 
}

RamData WindowsMemoryCollector::collectMemoryData() {
    RamData data;
    
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (!GlobalMemoryStatusEx(&memStatus)) {
        std::cerr << "Failed to retrieve memory metrics." << std::endl;
        return data;
    }
    
    // Static info
    data.info.totalMemoryBytes = memStatus.ullTotalPhys;
    data.info.ramType = ramType;
    
    // Dynamic metrics
    unsigned long long usedMemoryBytes = memStatus.ullTotalPhys - memStatus.ullAvailPhys;
    
    data.metrics.usedMemoryMB = usedMemoryBytes / (1024 * 1024);
    data.metrics.availableMemoryMB = memStatus.ullAvailPhys / (1024 * 1024);
    data.metrics.usagePercentage = (static_cast<double>(usedMemoryBytes) / memStatus.ullTotalPhys) * 100.0;
    
    return data;
}

#endif // _WIN32
