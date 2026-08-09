#include "CpuData.hpp"
#include "MockCpuCollector.hpp"

MockCpuCollector::MockCpuCollector() {
    // Initialize mock data with some default values
    mockData.info.cpuName = "Mock CPU";
    mockData.info.physicalCores = 4;
    mockData.info.logicalThreads = 8;
    mockData.info.baseFrequencyGHz = 2.5;
    mockData.info.maxFrequencyGHz = 4.0;

    mockData.metrics.usage = 50.0; // 50% usage
    mockData.metrics.currentFrequency = 3.0; // 3 GHz
}

CpuData MockCpuCollector::collectCpuData() {
    // Return the mock data
    return mockData;
}