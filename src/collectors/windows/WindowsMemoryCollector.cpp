#ifdef _WIN32
#include "collectors/windows/WindowsMemoryCollector.hpp"
#include "collectors/windows/SmbiosMemory.hpp"
#include <cstring>
#include <vector>

WindowsMemoryCollector::WindowsMemoryCollector() { readMemoryInfo(); }
void WindowsMemoryCollector::readMemoryInfo() {
    constexpr DWORD rsmb = 0x52534D42; // Windows 'RSMB' firmware provider.
    const UINT size = GetSystemFirmwareTable(rsmb, 0, nullptr, 0);
    if (size < 8) return;
    std::vector<std::uint8_t> raw(size);
    if (GetSystemFirmwareTable(rsmb, 0, raw.data(), size) != size) return;
    DWORD tableSize = 0;
    std::memcpy(&tableSize, raw.data() + 4, sizeof(tableSize));
    if (tableSize > raw.size() - 8) return;
    ramType = parseSmbiosMemoryType(raw.data() + 8, tableSize);
}
RamData WindowsMemoryCollector::collectMemoryData() {
    RamData data{};
    data.info.ramType = ramType;
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status) || status.ullTotalPhys == 0) return data;
    data.info.totalMemoryBytes = status.ullTotalPhys;
    const auto used = status.ullTotalPhys - status.ullAvailPhys;
    data.metrics.usedMemoryMB = used / (1024 * 1024);
    data.metrics.availableMemoryMB = status.ullAvailPhys / (1024 * 1024);
    data.metrics.usagePercentage = static_cast<double>(used) / status.ullTotalPhys * 100.0;
    return data;
}
#endif
