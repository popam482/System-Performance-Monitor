#pragma once
#include <string>

struct CpuInfo{
    std::string cpuName;
    unsigned int physicalCores;
    unsigned int logicalThreads;

    double baseFrequencyGHz;
    double maxFrequencyGHz;
};