#ifdef _WIN32
#include "collectors/windows/WindowsCpuCollector.hpp"
#include <windows.h>
#include <powrprof.h>
#include <pdhmsg.h>
#include <algorithm>
#include <iostream>
#include <vector>
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#elif defined(__i386__) || defined(__x86_64__)
#include <cpuid.h>
#endif

namespace {
// Layout documented by CallNtPowerInformation; absent from some SDK headers.
struct ProcessorPowerInformation {
    ULONG number, maxMhz, currentMhz, mhzLimit, maxIdleState, currentIdleState;
};

std::optional<double> readBaseFrequency() {
    // Intel CPUID.16H reports specification data, not a live clock.
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    int registers[4]{};
    __cpuid(registers, 0);
    if (registers[0] < 0x16 || registers[1] != 0x756e6547 ||
        registers[3] != 0x49656e69 || registers[2] != 0x6c65746e) return std::nullopt;
    __cpuid(registers, 0x16);
    const unsigned int mhz = static_cast<unsigned int>(registers[0]) & 0xFFFF;
    if (mhz > 0) return mhz / 1000.0;
#elif defined(__i386__) || defined(__x86_64__)
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    if (!__get_cpuid(0, &eax, &ebx, &ecx, &edx) || eax < 0x16 ||
        ebx != 0x756e6547 || edx != 0x49656e69 || ecx != 0x6c65746e) return std::nullopt;
    if (__get_cpuid(0x16, &eax, &ebx, &ecx, &edx) && (eax & 0xFFFF) > 0)
        return (eax & 0xFFFF) / 1000.0;
#endif
    return std::nullopt;
}
}

WindowsCpuCollector::WindowsCpuCollector() {
    readCpuInfo(); // Hardware identity is independent of PDH availability.
    if (!initializePDH()) std::cerr << "CPU usage counter unavailable.\n";
}
WindowsCpuCollector::~WindowsCpuCollector() { cleanupPDH(); }

bool WindowsCpuCollector::initializePDH() {
    if (PdhOpenQueryA(nullptr, 0, &cpuQuery) != ERROR_SUCCESS) return false;
    if (PdhAddEnglishCounterA(cpuQuery, "\\Processor(_Total)\\% Processor Time",
                             0, &cpuTotalHandle) != ERROR_SUCCESS ||
        PdhCollectQueryData(cpuQuery) != ERROR_SUCCESS) {
        cleanupPDH();
        return false;
    }
    return true;
}
void WindowsCpuCollector::cleanupPDH() {
    if (cpuQuery) PdhCloseQuery(cpuQuery);
    cpuQuery = nullptr;
    cpuTotalHandle = nullptr;
}
CpuData WindowsCpuCollector::collectCpuData() {
    CpuData data{};
    data.info.cpuName = cpuName;
    data.info.physicalCores = physicalCores;
    data.info.logicalThreads = logicalThreads;
    data.info.baseFrequencyGHz = baseFrequency;
    // Registry ~MHz is not a reliable nominal/base-frequency source.
    readFrequencies(data);
    if (cpuQuery && PdhCollectQueryData(cpuQuery) == ERROR_SUCCESS)
        data.metrics.usage = getCpuUsage();
    return data;
}
std::optional<double> WindowsCpuCollector::getCpuUsage() {
    PDH_FMT_COUNTERVALUE value{};
    if (PdhGetFormattedCounterValue(cpuTotalHandle, PDH_FMT_DOUBLE, nullptr, &value)
            == ERROR_SUCCESS &&
        (value.CStatus == PDH_CSTATUS_VALID_DATA || value.CStatus == PDH_CSTATUS_NEW_DATA))
        return value.doubleValue;
    return std::nullopt;
}
void WindowsCpuCollector::readFrequencies(CpuData& data) {
    // The legacy power API has no group selector. Avoid presenting a partial
    // processor group as a system-wide measurement on large machines.
    if (!logicalThreads || GetActiveProcessorGroupCount() != 1) return;
    std::vector<ProcessorPowerInformation> power(*logicalThreads);
    if (CallNtPowerInformation(ProcessorInformation, nullptr, 0, power.data(),
            static_cast<ULONG>(power.size() * sizeof(ProcessorPowerInformation))) != 0) return;
    ULONG maxMhz = 0;
    double currentMhz = 0.0;
    bool allCurrentValid = true, allMaxValid = true;
    for (const auto& processor : power) {
        allMaxValid = allMaxValid && processor.maxMhz > 0;
        allCurrentValid = allCurrentValid && processor.currentMhz > 0;
        maxMhz = (std::max)(maxMhz, processor.maxMhz);
        currentMhz += processor.currentMhz;
    }
    if (allMaxValid) data.info.maxFrequencyGHz = maxMhz / 1000.0;
    if (allCurrentValid) data.metrics.currentFrequency = currentMhz / power.size() / 1000.0;
}
void WindowsCpuCollector::readCpuInfo() {
    baseFrequency = readBaseFrequency();
    const DWORD active = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    if (active != 0) logicalThreads = active;
    DWORD bytes = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bytes);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && bytes > 0) {
        std::vector<BYTE> buffer(bytes);
        if (GetLogicalProcessorInformationEx(RelationProcessorCore,
                reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &bytes)) {
            unsigned int cores = 0;
            size_t offset = 0;
            while (offset + sizeof(DWORD) * 2 <= bytes) {
                const auto* entry = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
                    buffer.data() + offset);
                if (entry->Size < sizeof(DWORD) * 2 || entry->Size > bytes - offset) break;
                if (entry->Relationship == RelationProcessorCore) ++cores;
                offset += entry->Size;
            }
            if (offset == bytes && cores > 0) physicalCores = cores;
        }
    }
    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &key) == ERROR_SUCCESS) {
        char name[256]{};
        DWORD size = sizeof(name) - 1, type = 0;
        if (RegQueryValueExA(key, "ProcessorNameString", nullptr, &type,
                reinterpret_cast<LPBYTE>(name), &size) == ERROR_SUCCESS && type == REG_SZ)
            cpuName = name;
        RegCloseKey(key);
    }
}
#endif
