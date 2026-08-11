#ifdef _WIN32

#include "../../../include/collectors/windows/WindowsCpuCollector.hpp"
#include <iostream>
#include <thread>
#include <windows.h>
#include <chrono>

WindowsCpuCollector::WindowsCpuCollector() {
    if(initializePDH()) {
        readCpuInfo();
    } else {
        std::cerr << "Failed to initialize PDH for CPU metrics collection." << std::endl;
    }
}

WindowsCpuCollector::~WindowsCpuCollector() {
    cleanupPDH();
}

bool WindowsCpuCollector::initializePDH() {
    PDH_STATUS status = PdhOpenQuery(NULL, 0, &cpuQuery);
    if (status != ERROR_SUCCESS) {
        std::cerr << "PdhOpenQuery failed with status: " << status << std::endl;
        return false;
    }

    // Add total CPU usage counter
    status = PdhAddCounter(cpuQuery, "\\Processor(_Total)\\% Processor Time", 0, &cpuTotalHandle);
    if (status != ERROR_SUCCESS) {
        std::cerr << "PdhAddCounter for total CPU usage failed with status: " << status << std::endl;
        PdhCloseQuery(cpuQuery);
        return false;
    }

    // Add per-core CPU usage counters
    logicalThreads = std::thread::hardware_concurrency();
    for (unsigned int i = 0; i < logicalThreads; i++) {
        std::string counterPath = "\\Processor(" + std::to_string(i) + ")\\% Processor Time";
        PDH_HCOUNTER handle;
        status = PdhAddCounter(cpuQuery, counterPath.c_str(), 0, &handle);
        if (status == ERROR_SUCCESS) {
            perCoreHandles.push_back(handle);
        }
    }

    // PDH requires an initial data collection to start measuring
    status = PdhCollectQueryData(cpuQuery);
    if (status != ERROR_SUCCESS) {
        std::cerr << "Failed to collect initial query data: " << status << std::endl;
        return false;
    }

    // Wait before collecting data again (PDH necesită timp între măsurători)
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    return true;
}

void WindowsCpuCollector::cleanupPDH() {
    if (cpuQuery != NULL) {
        PdhCloseQuery(cpuQuery);
        cpuQuery = NULL;
    }
}

CpuData WindowsCpuCollector::collectCpuData() {
    CpuData data;

    PDH_STATUS status = PdhCollectQueryData(cpuQuery);
    if (status != ERROR_SUCCESS) {
        std::cerr << "Failed to collect query data: " << status << std::endl;
        return data;
    }

    // Static data
    data.info.cpuName = cpuName;
    data.info.physicalCores = physicalCores;
    data.info.logicalThreads = logicalThreads;
    data.info.baseFrequencyGHz = baseFrequency;
    data.info.maxFrequencyGHz = maxFrequency;

    // Dynamic data
    data.metrics.usage = getCpuUsage();
    data.metrics.currentFrequency = getCurrentFrequency();

    return data;
}

double WindowsCpuCollector::getCpuUsage() {
    PDH_FMT_COUNTERVALUE counterValue;
    PDH_STATUS status = PdhGetFormattedCounterValue(cpuTotalHandle, PDH_FMT_DOUBLE, NULL, &counterValue);
    
    if (status == ERROR_SUCCESS) {
        return counterValue.doubleValue; 
    }
    
    return 0.0;
}

double WindowsCpuCollector::getCurrentFrequency() {
   
    return maxFrequency;
}

void WindowsCpuCollector::readCpuInfo() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    logicalThreads = sysInfo.dwNumberOfProcessors;
    physicalCores = (logicalThreads > 1) ? logicalThreads / 2 : 1;

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        char processorName[256] = {0};
        DWORD size = sizeof(processorName);
        
        if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, 
            (LPBYTE)processorName, &size) == ERROR_SUCCESS) {
            cpuName = processorName;
        }


        DWORD mhz = 0;
        size = sizeof(mhz);
        if (RegQueryValueExA(hKey, "~MHz", NULL, NULL, 
            (LPBYTE)&mhz, &size) == ERROR_SUCCESS) {
            baseFrequency = mhz / 1000.0; 
        }

        RegCloseKey(hKey);
    }

    if (cpuName.empty()) {
        cpuName = "Unknown Processor";
    }
    if (baseFrequency <= 0.0) {
        baseFrequency = 2.0;
    }

    maxFrequency = baseFrequency * 1.5; 
}

#endif // _WIN32
