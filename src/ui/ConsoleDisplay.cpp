#include "ui/ConsoleDisplay.hpp"

#include <iomanip>
#include <iostream>

void ConsoleDisplay::renderProgressBar(double percentage, int width) {
    if (percentage < 0.0) percentage = 0.0;
    if (percentage > 100.0) percentage = 100.0;

    const int pos = static_cast<int>((percentage / 100.0) * width);
    std::cout << "[";
    for (int i = 0; i < width; ++i) {
        if (i < pos) std::cout << '=';
        else if (i == pos) std::cout << '>';
        else std::cout << ' ';
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << percentage << "%";
}

void ConsoleDisplay::renderDashboard(const CpuData& cpu, const RamData& ram, const GpuData& gpu, const DiskData& disk) {
    std::cout << "\n=======================================================================\n";
    std::cout << "                   SYSTEM PERFORMANCE MONITOR (LIVE)                  \n";
    std::cout << "=======================================================================\n\n";

    std::cout << "[ CPU ] " << (cpu.info.cpuName.empty() ? "(unknown)" : cpu.info.cpuName) << "\n";
    std::cout << " Cores/Threads : " << cpu.info.physicalCores << " / " << cpu.info.logicalThreads << "\n";
    std::cout << " Usage         : ";
    renderProgressBar(cpu.metrics.usage);
    std::cout << "\n\n";

    std::cout << "[ RAM ]\n";
    std::cout << " Total / Used  : " << std::fixed << std::setprecision(2)
              << (ram.info.totalMemoryBytes / 1024.0 / 1024.0 / 1024.0) << " GB / "
              << (ram.metrics.usedMemoryMB / 1024.0) << " GB\n";
    std::cout << " Usage         : ";
    renderProgressBar(ram.metrics.usagePercentage);
    std::cout << "\n\n";

    std::cout << "[ GPU ] " << (gpu.info.gpuName.empty() ? "(disabled in this build)" : gpu.info.gpuName) << "\n";
    std::cout << " Usage         : ";
    renderProgressBar(gpu.metrics.usagePercentage);
    std::cout << "\n\n";

    std::cout << "[ Disk ] " << (disk.info.driveLetter.empty() ? "(unknown)" : disk.info.driveLetter) << "\n";
    std::cout << " Total / Used  : " << std::fixed << std::setprecision(2)
              << disk.info.totalSpaceGB << " GB / "
              << disk.metrics.usedSpaceGB << " GB\n";
    std::cout << " Usage         : ";
    renderProgressBar(disk.metrics.usagePercentage);
    std::cout << "\n\n";

    std::cout << "=======================================================================\n";
    std::cout << " Ctrl+C to stop\n" << std::flush;
}
