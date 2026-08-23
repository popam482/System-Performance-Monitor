#ifdef _WIN32

#include "collectors/windows/WindowsDiskCollector.hpp"
#include <windows.h>

WindowsDiskCollector::WindowsDiskCollector(std::string drive) : drivePath(drive) {}

DiskData WindowsDiskCollector::collectDiskData() {
    DiskData data;
    data.info.driveLetter = drivePath;

    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;

    if (GetDiskFreeSpaceExA(drivePath.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        double totalGB = static_cast<double>(totalBytes.QuadPart) / (1024.0 * 1024.0 * 1024.0);
        double freeGB = static_cast<double>(totalFreeBytes.QuadPart) / (1024.0 * 1024.0 * 1024.0);
        double usedGB = totalGB - freeGB;

        data.info.totalSpaceGB = totalGB;
        data.metrics.freeSpaceGB = freeGB;
        data.metrics.usedSpaceGB = usedGB;
        data.metrics.usagePercentage = (usedGB / totalGB) * 100.0;
    }

    return data;
}

#endif